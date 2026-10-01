/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// Encoder interfaces. Everything in here is header-only, so encoder plugins
// only depend on QtCore and their codec library.
//
// Encoders are provided by factories. Factories for codecs with external
// libraries live in plugins (Qt plugins, see EncoderRegistry): if the codec
// library is missing at runtime, the plugin simply does not load and the
// format is not offered. The plugins are compiled against the real codec
// headers, so there are no hand written prototypes or struct copies.

#include <QByteArrayView>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QtEndian>
#include <QtPlugin>

#include <algorithm>
#include <memory>
#include <vector>

namespace Audex::Encoding
{

// Input format: 44.1 kHz, 16 bit, stereo, little endian, whole frames
inline constexpr int SampleRate = 44100;
inline constexpr int Channels = 2;
inline constexpr int BytesPerFrame = 4;

// Encoders run in a worker thread. open(), write() and finish()/abort() are
// always called from that same thread, in this order.
class AudioEncoder
{
public:
    virtual ~AudioEncoder() = default;

    // totalFrames: number of stereo frames that will be written (exact)
    virtual bool open(const QString &path, qint64 totalFrames, QString *error) = 0;
    virtual bool write(QByteArrayView pcm, QString *error) = 0;
    virtual bool finish(QString *error) = 0; // file complete and closed
    virtual void abort() = 0; // close without caring about the result

    // Thread-safe. A blocking write() or finish() should return false soon.
    virtual void interrupt()
    {
    }

    // Track data of the file, by placeholder name (utils/encodercommand.h).
    // Called before open(); only the external encoder uses it, to fill in
    // $ttitle and friends.
    virtual void setTrackValues(const QMap<QString, QString> &values)
    {
        Q_UNUSED(values)
    }
};

// Reads a file of a lossless format back, e.g. to repair it. The PCM has the
// format of AudioEncoder's input.
class AudioDecoder
{
public:
    virtual ~AudioDecoder() = default;

    virtual bool open(const QString &path, QString *error) = 0;
    virtual qint64 totalFrames() const = 0; // after open()
    // Next part of the audio, whole frames; empty at the end of the file
    virtual bool read(QByteArray &pcm, QString *error) = 0;
};

struct EncoderOption {
    enum class Type {
        Integer,
        Boolean,
        Choice,
        Text
    };

    QString key;
    QString label;
    Type type = Type::Integer;
    QVariant defaultValue;
    int minimum = 0;
    int maximum = 0;
    QStringList choices; // Choice: stored values
    QStringList choiceLabels; // Choice: shown texts (same order)
    QString toolTip;
};

class EncoderFactory
{
public:
    virtual ~EncoderFactory() = default;

    virtual QString id() const = 0; // "flac"
    virtual QString displayName() const = 0; // "FLAC"
    virtual QString fileSuffix(const QVariantMap &settings) const = 0; // "flac"
    virtual QString version() const = 0; // library version for the log
    virtual QList<EncoderOption> options() const = 0;
    virtual std::unique_ptr<AudioEncoder> create(const QVariantMap &settings) const = 0;

    // Lossless formats can read their files back; nullptr otherwise
    virtual std::unique_ptr<AudioDecoder> createDecoder() const
    {
        return nullptr;
    }
};

// ---- helpers for implementations ------------------------------------------------

inline QVariant optionValue(const QList<EncoderOption> &options, const QVariantMap &settings, const QString &key)
{
    const auto it = settings.constFind(key);
    for (const EncoderOption &o : options) {
        if (o.key != key)
            continue;
        if (it == settings.cend())
            return o.defaultValue;
        switch (o.type) {
        case EncoderOption::Type::Integer: {
            bool ok = false;
            const int v = it.value().toInt(&ok);
            return ok ? QVariant(std::clamp(v, o.minimum, o.maximum)) : o.defaultValue;
        }
        case EncoderOption::Type::Boolean:
            return it.value().toBool();
        case EncoderOption::Type::Choice:
            return o.choices.contains(it.value().toString()) ? it.value() : o.defaultValue;
        case EncoderOption::Type::Text:
            return it.value().toString();
        }
    }
    return it == settings.cend() ? QVariant() : it.value();
}

// Little endian bytes to native int16 samples (copy: the input may be unaligned)
inline void toInt16(QByteArrayView pcm, std::vector<qint16> &out)
{
    const qsizetype samples = pcm.size() / 2;
    out.resize(size_t(samples));
    for (qsizetype i = 0; i < samples; ++i)
        out[size_t(i)] = qFromLittleEndian<qint16>(pcm.constData() + 2 * i);
}

}

#define AudexEncoderFactory_iid "org.kde.audex.EncoderFactory/1.2"
Q_DECLARE_INTERFACE(Audex::Encoding::EncoderFactory, AudexEncoderFactory_iid)
