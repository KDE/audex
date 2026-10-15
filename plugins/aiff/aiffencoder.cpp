/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// AIFF: uncompressed PCM, big endian, with room for tags (an "ID3 " chunk,
// written by TagLib). Needs no codec library.

#include <QCoreApplication>
#include <QFile>
#include <QObject>

#include <cstring>

#include "encoding/encoder.h"

using namespace Qt::StringLiterals;
using namespace Audex::Encoding;

namespace
{

QString tr(const char *text)
{
    return QCoreApplication::translate("AiffEncoder", text);
}

bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

// 44100 as an 80 bit IEEE 754 extended float, as COMM stores the sample rate
constexpr char SampleRate80[10] = {'\x40', '\x0E', '\xAC', '\x44', 0, 0, 0, 0, 0, 0};

constexpr int HeaderSize = 54; // FORM, COMM, SSND up to the audio

// FORM/AIFF with COMM and the head of SSND for the given amount of audio
QByteArray aiffHeader(qint64 dataBytes)
{
    const quint32 data = quint32(std::min<qint64>(dataBytes, 0xFFFFFFFFll - HeaderSize));
    QByteArray h(HeaderSize, '\0');
    char *p = h.data();
    std::memcpy(p, "FORM", 4);
    qToBigEndian<quint32>(HeaderSize - 8 + data, p + 4);
    std::memcpy(p + 8, "AIFFCOMM", 8);
    qToBigEndian<quint32>(18, p + 16);
    qToBigEndian<quint16>(Channels, p + 20);
    qToBigEndian<quint32>(data / BytesPerFrame, p + 22);
    qToBigEndian<quint16>(16, p + 26);
    std::memcpy(p + 28, SampleRate80, 10);
    std::memcpy(p + 38, "SSND", 4);
    qToBigEndian<quint32>(8 + data, p + 42);
    // offset and block size (p + 46, p + 50) stay 0
    return h;
}

// 16 bit samples: little endian (CD) <-> big endian (AIFF), the same swap both ways
void swapSamples(const char *in, char *out, qsizetype bytes)
{
    for (qsizetype i = 0; i + 1 < bytes; i += 2) {
        out[i] = in[i + 1];
        out[i + 1] = in[i];
    }
}

class AiffEncoder : public AudioEncoder
{
public:
    bool open(const QString &path, qint64 totalFrames, QString *error) override
    {
        m_file.setFileName(path);
        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return fail(error, m_file.errorString());
        m_bytes = 0;
        const QByteArray h = aiffHeader(totalFrames * BytesPerFrame);
        if (m_file.write(h) != h.size())
            return fail(error, m_file.errorString());
        return true;
    }

    bool write(QByteArrayView pcm, QString *error) override
    {
        m_buffer.resize(pcm.size());
        swapSamples(pcm.constData(), m_buffer.data(), pcm.size());
        if (m_file.write(m_buffer) != m_buffer.size())
            return fail(error, m_file.errorString());
        m_bytes += pcm.size();
        return true;
    }

    bool finish(QString *error) override
    {
        const QByteArray h = aiffHeader(m_bytes); // in case fewer bytes arrived than announced
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
    QByteArray m_buffer;
    qint64 m_bytes = 0;
};

// 16 bit stereo PCM at 44.1 kHz, as AiffEncoder writes it; other chunks
// (tags) are skipped
class AiffDecoder : public AudioDecoder
{
public:
    bool open(const QString &path, QString *error) override
    {
        m_file.setFileName(path);
        if (!m_file.open(QIODevice::ReadOnly))
            return fail(error, m_file.errorString());
        const QByteArray form = m_file.read(12);
        if (form.size() != 12 || !form.startsWith("FORM") || form.mid(8, 4) != "AIFF")
            return fail(error, tr("%1 is not an AIFF file.").arg(path));
        bool format = false;
        while (true) {
            const QByteArray header = m_file.read(8);
            if (header.size() != 8)
                return fail(error, tr("%1 contains no audio data.").arg(path));
            const qint64 size = qFromBigEndian<quint32>(header.constData() + 4);
            if (header.startsWith("COMM")) {
                const QByteArray comm = m_file.read(size + (size & 1));
                format = comm.size() >= 18 && qFromBigEndian<quint16>(comm.constData()) == Channels && qFromBigEndian<quint16>(comm.constData() + 6) == 16
                    && std::memcmp(comm.constData() + 8, SampleRate80, 10) == 0;
            } else if (header.startsWith("SSND")) {
                if (!format)
                    return fail(error, tr("%1 is not 16 bit stereo PCM at 44.1 kHz.").arg(path));
                const QByteArray offset = m_file.read(8);
                if (offset.size() != 8 || !m_file.seek(m_file.pos() + qFromBigEndian<quint32>(offset.constData())))
                    return fail(error, tr("%1 contains no audio data.").arg(path));
                m_remaining = std::min(size - 8 - qint64(qFromBigEndian<quint32>(offset.constData())), m_file.size() - m_file.pos());
                m_remaining = std::max<qint64>(0, m_remaining);
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
        const QByteArray data = n > 0 ? m_file.read(n) : QByteArray();
        if (data.size() != n)
            return fail(error, m_file.errorString());
        pcm.resize(data.size());
        swapSamples(data.constData(), pcm.data(), data.size());
        m_remaining -= n;
        return true;
    }

private:
    QFile m_file;
    qint64 m_remaining = 0;
    qint64 m_frames = 0;
};

}

class AiffEncoderFactory : public QObject, public EncoderFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AudexEncoderFactory_iid FILE "aiff.json")
    Q_INTERFACES(Audex::Encoding::EncoderFactory)

public:
    QString id() const override
    {
        return u"aiff"_s;
    }
    QString displayName() const override
    {
        return u"AIFF"_s;
    }
    QString fileSuffix(const QVariantMap &) const override
    {
        return u"aiff"_s;
    }
    QString version() const override
    {
        return tr("built-in");
    }
    QList<EncoderOption> options() const override
    {
        return {};
    }
    std::unique_ptr<AudioEncoder> create(const QVariantMap &) const override
    {
        return std::make_unique<AiffEncoder>();
    }
    std::unique_ptr<AudioDecoder> createDecoder() const override
    {
        return std::make_unique<AiffDecoder>();
    }
};

#include "aiffencoder.moc"
