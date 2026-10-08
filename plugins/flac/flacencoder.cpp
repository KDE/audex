/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QCoreApplication>
#include <QFile>
#include <QObject>

#include <FLAC/metadata.h>
#include <FLAC/stream_decoder.h>
#include <FLAC/stream_encoder.h>

#include "encoding/encoder.h"

using namespace Qt::StringLiterals;
using namespace Audex::Encoding;

namespace
{

QString tr(const char *text)
{
    return QCoreApplication::translate("FlacEncoder", text);
}

QList<EncoderOption> flacOptions()
{
    EncoderOption level;
    level.key = u"compression"_s;
    level.label = tr("Compression level");
    level.type = EncoderOption::Type::Integer;
    level.defaultValue = 5;
    level.minimum = 0;
    level.maximum = 8;
    level.toolTip = tr("0 = fastest, 8 = smallest files. The audio data is identical for all levels.");

    EncoderOption verify;
    verify.key = u"verify"_s;
    verify.label = tr("Verify while encoding");
    verify.type = EncoderOption::Type::Boolean;
    verify.defaultValue = true;
    verify.toolTip = tr("Decode the output in parallel and compare it with the input.");

    return {level, verify};
}

class FlacEncoder : public AudioEncoder
{
public:
    FlacEncoder(unsigned level, bool verify)
        : m_level(level)
        , m_verify(verify)
    {
    }

    ~FlacEncoder() override
    {
        abort();
    }

    bool open(const QString &path, qint64 totalFrames, QString *error) override
    {
        m_encoder = FLAC__stream_encoder_new();
        if (!m_encoder)
            return fail(error, tr("Cannot create the FLAC encoder."));

        bool ok = FLAC__stream_encoder_set_verify(m_encoder, m_verify);
        ok &= FLAC__stream_encoder_set_compression_level(m_encoder, m_level);
        ok &= FLAC__stream_encoder_set_channels(m_encoder, Channels);
        ok &= FLAC__stream_encoder_set_bits_per_sample(m_encoder, 16);
        ok &= FLAC__stream_encoder_set_sample_rate(m_encoder, SampleRate);
        ok &= FLAC__stream_encoder_set_total_samples_estimate(m_encoder, FLAC__uint64(totalFrames));

        // Reserve space for tags and cover, so the tagger does not have to
        // rewrite the whole file. The encoder does not copy metadata objects;
        // they stay alive until finish().
        m_padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
        if (m_padding) {
            m_padding->length = 64 * 1024;
            ok &= FLAC__stream_encoder_set_metadata(m_encoder, &m_padding, 1);
        }
        if (!ok)
            return fail(error, tr("Cannot configure the FLAC encoder."));

        const QByteArray name = QFile::encodeName(path);
        const FLAC__StreamEncoderInitStatus status = FLAC__stream_encoder_init_file(m_encoder, name.constData(), nullptr, nullptr);
        if (status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
            return fail(error, tr("Cannot start the FLAC encoder: %1").arg(QString::fromLatin1(FLAC__StreamEncoderInitStatusString[status])));
        m_initialized = true;
        return true;
    }

    bool write(QByteArrayView pcm, QString *error) override
    {
        const qsizetype frames = pcm.size() / BytesPerFrame;
        m_buffer.resize(size_t(frames * Channels));
        for (qsizetype i = 0; i < frames * Channels; ++i)
            m_buffer[size_t(i)] = qFromLittleEndian<qint16>(pcm.constData() + 2 * i);
        if (!FLAC__stream_encoder_process_interleaved(m_encoder, m_buffer.data(), unsigned(frames)))
            return fail(error, stateError());
        return true;
    }

    bool finish(QString *error) override
    {
        if (!m_encoder)
            return fail(error, tr("The FLAC encoder is not open."));
        const bool ok = FLAC__stream_encoder_finish(m_encoder);
        const QString message = ok ? QString() : stateError();
        m_initialized = false;
        release();
        if (!ok)
            return fail(error, message);
        return true;
    }

    void abort() override
    {
        if (m_encoder && m_initialized)
            FLAC__stream_encoder_finish(m_encoder);
        m_initialized = false;
        release();
    }

private:
    static bool fail(QString *error, const QString &message)
    {
        if (error)
            *error = message;
        return false;
    }

    QString stateError() const
    {
        const FLAC__StreamEncoderState state = FLAC__stream_encoder_get_state(m_encoder);
        if (state == FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA)
            return tr("FLAC verification failed: the encoded audio differs from the input.");
        return tr("FLAC encoder error: %1").arg(QString::fromLatin1(FLAC__StreamEncoderStateString[state]));
    }

    void release()
    {
        if (m_encoder)
            FLAC__stream_encoder_delete(m_encoder);
        m_encoder = nullptr;
        if (m_padding)
            FLAC__metadata_object_delete(m_padding);
        m_padding = nullptr;
    }

    unsigned m_level;
    bool m_verify;
    FLAC__StreamEncoder *m_encoder = nullptr;
    FLAC__StreamMetadata *m_padding = nullptr;
    bool m_initialized = false;
    std::vector<FLAC__int32> m_buffer;
};

class FlacDecoder : public AudioDecoder
{
public:
    ~FlacDecoder() override
    {
        if (m_decoder)
            FLAC__stream_decoder_delete(m_decoder);
    }

    bool open(const QString &path, QString *error) override
    {
        m_decoder = FLAC__stream_decoder_new();
        if (!m_decoder)
            return fail(error, tr("Cannot create the FLAC decoder."));
        FLAC__stream_decoder_set_md5_checking(m_decoder, true);
        const QByteArray name = QFile::encodeName(path);
        if (FLAC__stream_decoder_init_file(m_decoder, name.constData(), writeCallback, metadataCallback, errorCallback, this)
            != FLAC__STREAM_DECODER_INIT_STATUS_OK)
            return fail(error, tr("Cannot open %1 for reading.").arg(path));
        if (!FLAC__stream_decoder_process_until_end_of_metadata(m_decoder) || !m_error.isEmpty())
            return fail(error, m_error.isEmpty() ? stateError() : m_error);
        if (!m_formatOk)
            return fail(error, tr("%1 is not 16 bit stereo audio at 44.1 kHz.").arg(path));
        return true;
    }

    qint64 totalFrames() const override
    {
        return m_frames;
    }

    bool read(QByteArray &pcm, QString *error) override
    {
        m_buffer.clear();
        while (m_buffer.isEmpty()) {
            if (FLAC__stream_decoder_get_state(m_decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
                // the MD5 checksum of the stream info is compared at the end
                if (!FLAC__stream_decoder_finish(m_decoder))
                    return fail(error, tr("FLAC decoding failed: the MD5 checksum does not match."));
                break;
            }
            if (!FLAC__stream_decoder_process_single(m_decoder) || !m_error.isEmpty())
                return fail(error, m_error.isEmpty() ? stateError() : m_error);
        }
        pcm = m_buffer;
        return true;
    }

private:
    static bool fail(QString *error, const QString &message)
    {
        if (error)
            *error = message;
        return false;
    }

    QString stateError() const
    {
        return tr("FLAC decoder error: %1").arg(QString::fromLatin1(FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(m_decoder)]));
    }

    static FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *data)
    {
        auto *self = static_cast<FlacDecoder *>(data);
        if (frame->header.channels != Channels || frame->header.bits_per_sample != 16) {
            self->m_error = tr("Unexpected FLAC frame format.");
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
        const qsizetype start = self->m_buffer.size();
        self->m_buffer.resize(start + qsizetype(frame->header.blocksize) * BytesPerFrame);
        char *out = self->m_buffer.data() + start;
        for (unsigned i = 0; i < frame->header.blocksize; ++i, out += BytesPerFrame) {
            qToLittleEndian<qint16>(qint16(buffer[0][i]), out);
            qToLittleEndian<qint16>(qint16(buffer[1][i]), out + 2);
        }
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    static void metadataCallback(const FLAC__StreamDecoder *, const FLAC__StreamMetadata *metadata, void *data)
    {
        auto *self = static_cast<FlacDecoder *>(data);
        if (metadata->type != FLAC__METADATA_TYPE_STREAMINFO)
            return;
        const FLAC__StreamMetadata_StreamInfo &info = metadata->data.stream_info;
        self->m_formatOk = info.channels == unsigned(Channels) && info.bits_per_sample == 16 && info.sample_rate == unsigned(SampleRate);
        self->m_frames = qint64(info.total_samples);
    }

    static void errorCallback(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus status, void *data)
    {
        static_cast<FlacDecoder *>(data)->m_error = tr("FLAC decoding error: %1").arg(QString::fromLatin1(FLAC__StreamDecoderErrorStatusString[status]));
    }

    FLAC__StreamDecoder *m_decoder = nullptr;
    QByteArray m_buffer;
    QString m_error;
    qint64 m_frames = 0;
    bool m_formatOk = false;
};

}

class FlacEncoderFactory : public QObject, public EncoderFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AudexEncoderFactory_iid FILE "flac.json")
    Q_INTERFACES(Audex::Encoding::EncoderFactory)

public:
    QString id() const override
    {
        return u"flac"_s;
    }
    QString displayName() const override
    {
        return u"FLAC"_s;
    }
    QString fileSuffix(const QVariantMap &) const override
    {
        return u"flac"_s;
    }
    QString version() const override
    {
        return u"libFLAC %1"_s.arg(QString::fromLatin1(FLAC__VERSION_STRING));
    }
    QList<EncoderOption> options() const override
    {
        return flacOptions();
    }
    std::unique_ptr<AudioEncoder> create(const QVariantMap &settings) const override
    {
        const QList<EncoderOption> o = options();
        return std::make_unique<FlacEncoder>(unsigned(optionValue(o, settings, u"compression"_s).toInt()), optionValue(o, settings, u"verify"_s).toBool());
    }
    std::unique_ptr<AudioDecoder> createDecoder() const override
    {
        return std::make_unique<FlacDecoder>();
    }
};

#include "flacencoder.moc"
