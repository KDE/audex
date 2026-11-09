/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QStringList>

#include <memory>

#include "encoder.h"

namespace Audex::Encoding
{

// Knows all available encoders: the built-in ones (WAV, external command)
// and those from plugins. Plugins are looked up in
//   $AUDEX_ENCODER_PLUGIN_PATH (colon separated)
//   <application dir>/plugins/audex_encoders
//   <Qt plugin paths>/audex_encoders
class EncoderRegistry
{
public:
    EncoderRegistry(); // built-in encoders only

    void loadPlugins(const QStringList &directories = defaultPluginDirectories());
    void addFactory(std::shared_ptr<EncoderFactory> factory); // replaces one with the same id

    EncoderFactory *factory(const QString &id) const;

    // One line per plugin: loaded (with library version) or why it failed
    QStringList diagnostics() const
    {
        return m_diagnostics;
    }

    static QStringList defaultPluginDirectories();

private:
    QList<std::shared_ptr<EncoderFactory>> m_factories;
    QStringList m_diagnostics;
};

// ---- built-in encoders ------------------------------------------------------------

// Plain RIFF WAVE (no tags)
class WavEncoderFactory : public EncoderFactory
{
public:
    QString id() const override;
    QString displayName() const override;
    QString fileSuffix(const QVariantMap &settings) const override;
    QString version() const override;
    QList<EncoderOption> options() const override;
    std::unique_ptr<AudioEncoder> create(const QVariantMap &settings) const override;
    std::unique_ptr<AudioDecoder> createDecoder() const override;
};

#ifndef AUDEX_SANDBOX
// Any command line encoder that reads WAVE from stdin, e.g.
//   fdkaac --silent -m 5 -o $o -
//   ffmpeg -loglevel error -f wav -i - -c:a alac $o
// The setting "commandArgs" holds the arguments of parseCommandScheme()
// (utils/encodercommand.h), already split: values in them can no longer
// change the command line. $o and the track values are filled in when the
// output file is opened. The WAVE header carries the exact length, so
// encoders that need it work as well.
class ExternalEncoderFactory : public EncoderFactory
{
public:
    QString id() const override;
    QString displayName() const override;
    QString fileSuffix(const QVariantMap &settings) const override;
    QString version() const override;
    QList<EncoderOption> options() const override;
    std::unique_ptr<AudioEncoder> create(const QVariantMap &settings) const override;
};
#endif

// 44 byte RIFF header for 16 bit stereo PCM
QByteArray wavHeader(qint64 dataBytes);

}
