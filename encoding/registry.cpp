/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "registry.h"

#include "utils/encodercommand.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QLibrary>
#include <QPluginLoader>
#include <QProcess>

#include <algorithm>
#include <atomic>

using namespace Qt::StringLiterals;

namespace Audex::Encoding
{

namespace
{

QString tr(const char *text)
{
    return QCoreApplication::translate("Audex::Encoding", text);
}

bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

void putLe32(char *p, quint32 v)
{
    qToLittleEndian<quint32>(v, p);
}

}

QByteArray wavHeader(qint64 dataBytes)
{
    QByteArray h(44, '\0');
    char *p = h.data();
    const quint32 data = quint32(std::min<qint64>(dataBytes, 0xFFFFFFFFll - 36));
    std::memcpy(p, "RIFF", 4);
    putLe32(p + 4, 36 + data);
    std::memcpy(p + 8, "WAVEfmt ", 8);
    putLe32(p + 16, 16);
    qToLittleEndian<quint16>(1, p + 20);
    qToLittleEndian<quint16>(Channels, p + 22);
    putLe32(p + 24, SampleRate);
    putLe32(p + 28, SampleRate * BytesPerFrame);
    qToLittleEndian<quint16>(BytesPerFrame, p + 32);
    qToLittleEndian<quint16>(16, p + 34);
    std::memcpy(p + 36, "data", 4);
    putLe32(p + 40, data);
    return h;
}

// ---- WAV ------------------------------------------------------------------------------

namespace
{

class WavEncoder : public AudioEncoder
{
public:
    bool open(const QString &path, qint64 totalFrames, QString *error) override
    {
        m_file.setFileName(path);
        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return fail(error, m_file.errorString());
        m_bytes = 0;
        const QByteArray h = wavHeader(totalFrames * BytesPerFrame);
        if (m_file.write(h) != h.size())
            return fail(error, m_file.errorString());
        return true;
    }

    bool write(QByteArrayView pcm, QString *error) override
    {
        if (m_file.write(pcm.constData(), pcm.size()) != pcm.size())
            return fail(error, m_file.errorString());
        m_bytes += pcm.size();
        return true;
    }

    bool finish(QString *error) override
    {
        const QByteArray h = wavHeader(m_bytes); // in case fewer bytes arrived than announced
        const bool ok = m_file.seek(0) && m_file.write(h) == h.size() && m_file.flush();
        const QString message = m_file.errorString();
        m_file.close();
        return ok || fail(error, message);
    }

    void abort() override
    {
        m_file.close();
    }

private:
    QFile m_file;
    qint64 m_bytes = 0;
};

}

namespace
{

// RIFF WAVE with 16 bit stereo PCM at 44.1 kHz, as WavEncoder writes it
class WavDecoder : public AudioDecoder
{
public:
    bool open(const QString &path, QString *error) override
    {
        m_file.setFileName(path);
        if (!m_file.open(QIODevice::ReadOnly))
            return fail(error, m_file.errorString());
        const QByteArray riff = m_file.read(12);
        if (riff.size() != 12 || !riff.startsWith("RIFF") || riff.mid(8, 4) != "WAVE")
            return fail(error, tr("%1 is not a WAVE file.").arg(path));
        bool format = false;
        while (true) {
            const QByteArray header = m_file.read(8);
            if (header.size() != 8)
                return fail(error, tr("%1 contains no audio data.").arg(path));
            const qint64 size = qFromLittleEndian<quint32>(header.constData() + 4);
            if (header.startsWith("fmt ")) {
                const QByteArray fmt = m_file.read(size + (size & 1));
                format = fmt.size() >= 16 && qFromLittleEndian<quint16>(fmt.constData()) == 1 && qFromLittleEndian<quint16>(fmt.constData() + 2) == Channels
                    && qFromLittleEndian<quint32>(fmt.constData() + 4) == quint32(SampleRate) && qFromLittleEndian<quint16>(fmt.constData() + 14) == 16;
            } else if (header.startsWith("data")) {
                if (!format)
                    return fail(error, tr("%1 is not 16 bit stereo PCM at 44.1 kHz.").arg(path));
                m_remaining = std::min(size, m_file.size() - m_file.pos());
                m_frames = m_remaining / BytesPerFrame;
                return true;
            } else if (!m_file.seek(m_file.pos() + size + (size & 1))) {
                return fail(error, m_file.errorString());
            }
        }
    }

    qint64 totalFrames() const override
    {
        return m_frames;
    }

    bool read(QByteArray &pcm, QString *error) override
    {
        const qint64 n = std::min<qint64>(m_remaining - m_remaining % BytesPerFrame, 1 << 20);
        pcm = n > 0 ? m_file.read(n) : QByteArray();
        if (pcm.size() != n)
            return fail(error, m_file.errorString());
        m_remaining -= n;
        return true;
    }

private:
    QFile m_file;
    qint64 m_remaining = 0;
    qint64 m_frames = 0;
};

}

QString WavEncoderFactory::id() const
{
    return u"wav"_s;
}
QString WavEncoderFactory::displayName() const
{
    return u"WAVE"_s;
}
QString WavEncoderFactory::fileSuffix(const QVariantMap &) const
{
    return u"wav"_s;
}
QString WavEncoderFactory::version() const
{
    return tr("built-in");
}
QList<EncoderOption> WavEncoderFactory::options() const
{
    return {};
}
std::unique_ptr<AudioEncoder> WavEncoderFactory::create(const QVariantMap &) const
{
    return std::make_unique<WavEncoder>();
}

std::unique_ptr<AudioDecoder> WavEncoderFactory::createDecoder() const
{
    return std::make_unique<WavDecoder>();
}

// ---- external command ----------------------------------------------------------------

namespace
{

class ExternalEncoder : public AudioEncoder
{
public:
    explicit ExternalEncoder(QStringList arguments)
        : m_arguments(std::move(arguments))
    {
    }

    ~ExternalEncoder() override
    {
        abort();
    }

    void setTrackValues(const QMap<QString, QString> &values) override
    {
        m_trackValues = values;
    }

    bool open(const QString &path, qint64 totalFrames, QString *error) override
    {
        // QProcess is created here: it must live in the worker thread
        const QStringList arguments = substituteValues(m_arguments, m_trackValues);
        const QStringList cmd = ExternalEncoderFactory::commandLine(arguments, path);
        if (cmd.isEmpty())
            return fail(error, tr("No encoder command configured."));
        if (!hasOutputFilePlaceholder(m_arguments))
            return fail(error, tr("The encoder command must contain $o for the output file."));

        m_process = std::make_unique<QProcess>();
        m_process->setStandardOutputFile(QProcess::nullDevice());
        m_process->start(cmd.first(), cmd.mid(1));
        if (!m_process->waitForStarted(10000))
            return fail(error, tr("Cannot start \"%1\": %2").arg(cmd.first(), m_process->errorString()));
        return send(wavHeader(totalFrames * BytesPerFrame), error);
    }

    bool write(QByteArrayView pcm, QString *error) override
    {
        return send(pcm, error);
    }

    bool finish(QString *error) override
    {
        if (!m_process)
            return fail(error, tr("The encoder is not running."));
        m_process->closeWriteChannel();
        while (!m_process->waitForFinished(100) && m_process->state() != QProcess::NotRunning) {
            if (m_interrupted) {
                abort();
                return fail(error, tr("Encoding was aborted."));
            }
            collectErrors();
        }
        collectErrors();
        const bool ok = m_process->exitStatus() == QProcess::NormalExit && m_process->exitCode() == 0;
        const QString message = tr("The encoder failed (exit code %1): %2").arg(m_process->exitCode()).arg(QString::fromLocal8Bit(m_stderr).trimmed());
        m_process.reset();
        return ok || fail(error, message);
    }

    void abort() override
    {
        if (!m_process)
            return;
        m_process->kill();
        m_process->waitForFinished(5000);
        m_process.reset();
    }

    void interrupt() override
    {
        m_interrupted = true;
    }

private:
    bool send(QByteArrayView data, QString *error)
    {
        if (m_process->write(data.constData(), data.size()) != data.size())
            return fail(error, tr("Cannot send data to the encoder: %1").arg(m_process->errorString()));
        while (m_process->bytesToWrite() > 0) {
            if (m_interrupted)
                return fail(error, tr("Encoding was aborted."));
            if (!m_process->waitForBytesWritten(100) && m_process->state() != QProcess::Running) {
                collectErrors();
                return fail(error, tr("The encoder stopped accepting data: %1 %2").arg(m_process->errorString(), QString::fromLocal8Bit(m_stderr).trimmed()));
            }
            collectErrors(); // keep the stderr pipe from filling up
        }
        return true;
    }

    void collectErrors()
    {
        m_stderr += m_process->readAllStandardError();
        if (m_stderr.size() > 16384)
            m_stderr = m_stderr.right(16384);
    }

    QStringList m_arguments;
    QMap<QString, QString> m_trackValues;
    std::unique_ptr<QProcess> m_process;
    QByteArray m_stderr;
    std::atomic_bool m_interrupted{false};
};

}

QString ExternalEncoderFactory::id() const
{
    return u"external"_s;
}
QString ExternalEncoderFactory::displayName() const
{
    return tr("External command");
}
QString ExternalEncoderFactory::fileSuffix(const QVariantMap &settings) const
{
    const QString s = optionValue(options(), settings, u"suffix"_s).toString().trimmed();
    return s.isEmpty() ? u"audio"_s : s;
}
QString ExternalEncoderFactory::version() const
{
    return tr("built-in");
}

QList<EncoderOption> ExternalEncoderFactory::options() const
{
    EncoderOption command;
    command.key = u"command"_s;
    command.label = tr("Command");
    command.type = EncoderOption::Type::Text;
    command.defaultValue = u"fdkaac --silent -m 5 -o %o -"_s;
    command.toolTip = tr("Reads WAVE from standard input. %o is the output file.");

    EncoderOption suffix;
    suffix.key = u"suffix"_s;
    suffix.label = tr("File suffix");
    suffix.type = EncoderOption::Type::Text;
    suffix.defaultValue = u"m4a"_s;

    return {command, suffix};
}

std::unique_ptr<AudioEncoder> ExternalEncoderFactory::create(const QVariantMap &settings) const
{
    return std::make_unique<ExternalEncoder>(arguments(settings));
}

QStringList ExternalEncoderFactory::arguments(const QVariantMap &settings) const
{
    const QStringList split = settings.value(u"commandArgs"_s).toStringList();
    if (!split.isEmpty())
        return split;
    return QProcess::splitCommand(optionValue(options(), settings, u"command"_s).toString());
}

QStringList ExternalEncoderFactory::commandLine(const QStringList &arguments, const QString &outputPath)
{
    QStringList args = arguments;
    for (QString &arg : args) {
        QString out;
        for (qsizetype i = 0; i < arg.size(); ++i) {
            if (arg.at(i) == u'%' && i + 1 < arg.size()) {
                const QChar next = arg.at(i + 1);
                if (next == u'o') {
                    out += outputPath;
                    ++i;
                    continue;
                }
                if (next == u'%') {
                    out += u'%';
                    ++i;
                    continue;
                }
            }
            out += arg.at(i);
        }
        arg = out;
    }
    return args;
}

// ---- registry ------------------------------------------------------------------------

EncoderRegistry::EncoderRegistry()
{
    addFactory(std::make_shared<WavEncoderFactory>());
    addFactory(std::make_shared<ExternalEncoderFactory>());
}

void EncoderRegistry::addFactory(std::shared_ptr<EncoderFactory> factory)
{
    for (auto &existing : m_factories) {
        if (existing->id() == factory->id()) {
            existing = std::move(factory);
            return;
        }
    }
    m_factories.append(std::move(factory));
}

QStringList EncoderRegistry::defaultPluginDirectories()
{
    QStringList dirs;
    const QString env = qEnvironmentVariable("AUDEX_ENCODER_PLUGIN_PATH");
    if (!env.isEmpty())
        dirs += env.split(u':', Qt::SkipEmptyParts);
    if (QCoreApplication::instance())
        dirs << QCoreApplication::applicationDirPath() + u"/plugins/audex_encoders"_s;
    for (const QString &path : QCoreApplication::libraryPaths())
        dirs << path + u"/audex_encoders"_s;
#ifdef AUDEX_ENCODER_PLUGIN_INSTALL_DIR
    dirs << QStringLiteral(AUDEX_ENCODER_PLUGIN_INSTALL_DIR); // where "make install" put them
#endif
    dirs.removeDuplicates();
    return dirs;
}

void EncoderRegistry::loadPlugins(const QStringList &directories)
{
    for (const QString &directory : directories) {
        const QDir dir(directory);
        if (!dir.exists())
            continue;
        for (const QString &name : dir.entryList(QDir::Files, QDir::Name)) {
            const QString path = dir.absoluteFilePath(name);
            if (!QLibrary::isLibrary(path))
                continue;
            QPluginLoader loader(path);
            if (loader.metaData().value("IID"_L1).toString() != QLatin1StringView(AudexEncoderFactory_iid)) {
                m_diagnostics << tr("%1: not an Audex encoder plugin").arg(name);
                continue;
            }
            QObject *instance = loader.instance(); // stays loaded
            auto *factory = qobject_cast<EncoderFactory *>(instance);
            if (!factory) {
                m_diagnostics << tr("%1: cannot be loaded (%2)").arg(name, loader.errorString());
                continue;
            }
            if (this->factory(factory->id())) {
                m_diagnostics << tr("%1: \"%2\" already loaded, skipped").arg(name, factory->id());
                continue;
            }
            // owned by the plugin loader, never deleted here
            addFactory(std::shared_ptr<EncoderFactory>(factory, [](EncoderFactory *) { }));
            m_diagnostics << tr("%1: %2 (%3)").arg(name, factory->displayName(), factory->version());
        }
    }
}

EncoderFactory *EncoderRegistry::factory(const QString &id) const
{
    for (const auto &f : m_factories)
        if (f->id() == id)
            return f.get();
    return nullptr;
}

}
