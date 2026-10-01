/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QCoreApplication>
#include <QFile>
#include <QObject>

#include <opusenc.h>

#include "encoding/encoder.h"

using namespace Qt::StringLiterals;
using namespace Audex::Encoding;

namespace
{

QString tr(const char *text)
{
    return QCoreApplication::translate("OpusEncoder", text);
}

QList<EncoderOption> opusOptions()
{
    EncoderOption bitrate;
    bitrate.key = u"bitrate"_s;
    bitrate.label = tr("Bitrate (kbit/s)");
    bitrate.type = EncoderOption::Type::Integer;
    bitrate.minimum = 32;
    bitrate.maximum = 510;
    bitrate.defaultValue = 160;
    bitrate.toolTip = tr("Variable bitrate target. 128–192 kbit/s is transparent for most music.");

    EncoderOption complexity;
    complexity.key = u"complexity"_s;
    complexity.label = tr("Complexity");
    complexity.type = EncoderOption::Type::Integer;
    complexity.minimum = 0;
    complexity.maximum = 10;
    complexity.defaultValue = 10;
    return {bitrate, complexity};
}

class AudexOpusEncoder : public AudioEncoder
{
public:
    AudexOpusEncoder(int bitrateKbps, int complexity)
        : m_bitrate(bitrateKbps)
        , m_complexity(complexity)
    {
    }

    ~AudexOpusEncoder() override
    {
        abort();
    }

    bool open(const QString &path, qint64 totalFrames, QString *error) override
    {
        Q_UNUSED(totalFrames)
        m_comments = ope_comments_create();
        if (!m_comments)
            return fail(error, tr("Cannot create the Opus comment block."));

        int status = OPE_OK;
        const QByteArray name = QFile::encodeName(path);
        m_encoder = ope_encoder_create_file(name.constData(), m_comments, SampleRate, Channels, 0, &status);
        if (!m_encoder)
            return fail(error, tr("Cannot create the Opus file: %1").arg(QString::fromUtf8(ope_strerror(status))));

        status = ope_encoder_ctl(m_encoder, OPUS_SET_BITRATE(m_bitrate * 1000));
        if (status == OPE_OK)
            status = ope_encoder_ctl(m_encoder, OPUS_SET_COMPLEXITY(m_complexity));
        if (status != OPE_OK)
            return fail(error, tr("Invalid Opus encoder settings: %1").arg(QString::fromUtf8(ope_strerror(status))));
        return true;
    }

    bool write(QByteArrayView pcm, QString *error) override
    {
        toInt16(pcm, m_samples);
        const int status = ope_encoder_write(m_encoder, m_samples.data(), int(pcm.size() / BytesPerFrame));
        if (status != OPE_OK)
            return fail(error, tr("Opus encoder error: %1").arg(QString::fromUtf8(ope_strerror(status))));
        return true;
    }

    bool finish(QString *error) override
    {
        if (!m_encoder)
            return fail(error, tr("The Opus encoder is not open."));
        const int status = ope_encoder_drain(m_encoder);
        release();
        if (status != OPE_OK)
            return fail(error, tr("Opus encoder error: %1").arg(QString::fromUtf8(ope_strerror(status))));
        return true;
    }

    void abort() override
    {
        release();
    }

private:
    static bool fail(QString *error, const QString &message)
    {
        if (error)
            *error = message;
        return false;
    }

    void release()
    {
        if (m_encoder)
            ope_encoder_destroy(m_encoder); // closes the file
        m_encoder = nullptr;
        if (m_comments)
            ope_comments_destroy(m_comments);
        m_comments = nullptr;
    }

    int m_bitrate;
    int m_complexity;
    OggOpusComments *m_comments = nullptr;
    OggOpusEnc *m_encoder = nullptr;
    std::vector<qint16> m_samples;
};

}

class OpusEncoderFactory : public QObject, public EncoderFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AudexEncoderFactory_iid FILE "opus.json")
    Q_INTERFACES(Audex::Encoding::EncoderFactory)

public:
    QString id() const override
    {
        return u"opus"_s;
    }
    QString displayName() const override
    {
        return u"Opus"_s;
    }
    QString fileSuffix(const QVariantMap &) const override
    {
        return u"opus"_s;
    }
    QString version() const override
    {
        return QString::fromUtf8(ope_get_version_string());
    }
    QList<EncoderOption> options() const override
    {
        return opusOptions();
    }
    std::unique_ptr<AudioEncoder> create(const QVariantMap &settings) const override
    {
        const QList<EncoderOption> o = options();
        return std::make_unique<AudexOpusEncoder>(optionValue(o, settings, u"bitrate"_s).toInt(), optionValue(o, settings, u"complexity"_s).toInt());
    }
};

#include "opusencoder.moc"
