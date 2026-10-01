/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QCoreApplication>
#include <QFile>
#include <QObject>

#include <lame/lame.h>

#include "encoding/encoder.h"

using namespace Qt::StringLiterals;
using namespace Audex::Encoding;

namespace
{

QString tr(const char *text)
{
    return QCoreApplication::translate("LameEncoder", text);
}

const QStringList Bitrates{u"128"_s, u"160"_s, u"192"_s, u"224"_s, u"256"_s, u"320"_s};

QList<EncoderOption> lameOptions()
{
    EncoderOption mode;
    mode.key = u"mode"_s;
    mode.label = tr("Mode");
    mode.type = EncoderOption::Type::Choice;
    mode.choices = {u"vbr"_s, u"cbr"_s};
    mode.choiceLabels = {tr("Variable bitrate"), tr("Constant bitrate")};
    mode.defaultValue = u"vbr"_s;

    EncoderOption quality;
    quality.key = u"vbrQuality"_s;
    quality.label = tr("VBR quality (V0 = best)");
    quality.type = EncoderOption::Type::Integer;
    quality.minimum = 0;
    quality.maximum = 9;
    quality.defaultValue = 0;
    quality.toolTip = tr("V0 ≈ 245 kbit/s, V2 ≈ 190 kbit/s, V5 ≈ 130 kbit/s");

    EncoderOption bitrate;
    bitrate.key = u"bitrate"_s;
    bitrate.label = tr("CBR bitrate (kbit/s)");
    bitrate.type = EncoderOption::Type::Choice;
    bitrate.choices = Bitrates;
    bitrate.choiceLabels = Bitrates;
    bitrate.defaultValue = u"320"_s;

    return {mode, quality, bitrate};
}

class LameEncoder : public AudioEncoder
{
public:
    LameEncoder(bool vbr, int vbrQuality, int bitrate)
        : m_vbr(vbr)
        , m_vbrQuality(vbrQuality)
        , m_bitrate(bitrate)
    {
    }

    ~LameEncoder() override
    {
        abort();
    }

    bool open(const QString &path, qint64 totalFrames, QString *error) override
    {
        // LAME cannot be re-initialized: one lame_t per file
        m_lame = lame_init();
        if (!m_lame)
            return fail(error, tr("Cannot create the LAME encoder."));

        lame_set_num_channels(m_lame, Channels);
        lame_set_in_samplerate(m_lame, SampleRate);
        lame_set_num_samples(m_lame, (unsigned long)totalFrames);
        lame_set_mode(m_lame, JOINT_STEREO);
        lame_set_quality(m_lame, 2);
        if (m_vbr) {
            lame_set_VBR(m_lame, vbr_default);
            lame_set_VBR_quality(m_lame, float(m_vbrQuality));
        } else {
            lame_set_VBR(m_lame, vbr_off);
            lame_set_brate(m_lame, m_bitrate);
        }
        // Tags are written by TagLib (ID3v2.4, UTF-8); the LAME/Xing frame
        // carries the gapless information.
        lame_set_write_id3tag_automatic(m_lame, 0);
        lame_set_bWriteVbrTag(m_lame, 1);

        if (lame_init_params(m_lame) < 0)
            return fail(error, tr("Invalid LAME encoder settings."));

        m_file.setFileName(path);
        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return fail(error, m_file.errorString());
        return true;
    }

    bool write(QByteArrayView pcm, QString *error) override
    {
        toInt16(pcm, m_samples);
        const int frames = int(pcm.size() / BytesPerFrame);
        m_output.resize(size_t(frames + frames / 4 + 7200)); // documented worst case
        const int bytes = lame_encode_buffer_interleaved(m_lame, m_samples.data(), frames, m_output.data(), int(m_output.size()));
        if (bytes < 0)
            return fail(error, tr("LAME encoder error %1.").arg(bytes));
        return writeBytes(bytes, error);
    }

    bool finish(QString *error) override
    {
        if (!m_lame)
            return fail(error, tr("The LAME encoder is not open."));
        m_output.resize(7200);
        const int bytes = lame_encode_flush(m_lame, m_output.data(), int(m_output.size()));
        if (bytes < 0)
            return fail(error, tr("LAME encoder error %1.").arg(bytes));
        if (!writeBytes(bytes, error))
            return false;

        // LAME/Xing tag: replaces the first (placeholder) frame
        std::vector<unsigned char> tag(4096);
        size_t tagSize = lame_get_lametag_frame(m_lame, tag.data(), tag.size());
        if (tagSize > tag.size()) {
            tag.resize(tagSize);
            tagSize = lame_get_lametag_frame(m_lame, tag.data(), tag.size());
        }
        if (tagSize > 0) {
            if (!m_file.seek(0) || m_file.write(reinterpret_cast<const char *>(tag.data()), qint64(tagSize)) != qint64(tagSize))
                return fail(error, m_file.errorString());
        }
        const bool ok = m_file.flush();
        m_file.close();
        release();
        if (!ok)
            return fail(error, tr("Cannot write the MP3 file."));
        return true;
    }

    void abort() override
    {
        if (m_file.isOpen())
            m_file.close();
        release();
    }

private:
    static bool fail(QString *error, const QString &message)
    {
        if (error)
            *error = message;
        return false;
    }

    bool writeBytes(int bytes, QString *error)
    {
        if (bytes > 0 && m_file.write(reinterpret_cast<const char *>(m_output.data()), bytes) != bytes)
            return fail(error, m_file.errorString());
        return true;
    }

    void release()
    {
        if (m_lame)
            lame_close(m_lame);
        m_lame = nullptr;
    }

    bool m_vbr;
    int m_vbrQuality;
    int m_bitrate;
    lame_t m_lame = nullptr;
    QFile m_file;
    std::vector<qint16> m_samples;
    std::vector<unsigned char> m_output;
};

}

class LameEncoderFactory : public QObject, public EncoderFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AudexEncoderFactory_iid FILE "mp3.json")
    Q_INTERFACES(Audex::Encoding::EncoderFactory)

public:
    QString id() const override
    {
        return u"mp3"_s;
    }
    QString displayName() const override
    {
        return u"MP3 (LAME)"_s;
    }
    QString fileSuffix(const QVariantMap &) const override
    {
        return u"mp3"_s;
    }
    QString version() const override
    {
        return u"LAME %1"_s.arg(QString::fromLatin1(get_lame_version()));
    }
    QList<EncoderOption> options() const override
    {
        return lameOptions();
    }
    std::unique_ptr<AudioEncoder> create(const QVariantMap &settings) const override
    {
        const QList<EncoderOption> o = options();
        return std::make_unique<LameEncoder>(optionValue(o, settings, u"mode"_s).toString() == u"vbr",
                                             optionValue(o, settings, u"vbrQuality"_s).toInt(),
                                             optionValue(o, settings, u"bitrate"_s).toInt());
    }
};

#include "lameencoder.moc"
