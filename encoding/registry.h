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

// Any command line encoder that reads WAVE from stdin, e.g.
//   fdkaac --silent -m 5 -o %o -
//   ffmpeg -loglevel error -f wav -i - -c:a alac %o
// %o is replaced by the output file, %% by a percent sign. The WAVE header
// carries the exact length, so encoders that need it work as well.
class ExternalEncoderFactory : public EncoderFactory
{
public:
    QString id() const override;
    QString displayName() const override;
    QString fileSuffix(const QVariantMap &settings) const override;
    QString version() const override;
    QList<EncoderOption> options() const override;
    std::unique_ptr<AudioEncoder> create(const QVariantMap &settings) const override;

    // "commandArgs" (a QStringList, already split: values in it can no longer
    // change the command line) or, if it is empty, the "command" string
    QStringList arguments(const QVariantMap &settings) const;

    // {"fdkaac", "-o", "%o", "-"} + path -> program and arguments
    static QStringList commandLine(const QStringList &arguments, const QString &outputPath);
};

// 44 byte RIFF header for 16 bit stereo PCM
QByteArray wavHeader(qint64 dataBytes);

}
