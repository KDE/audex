/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QMap>
#include <QString>

#include <KLocalizedString>

#include "utils/parameters.h"

#define ENCODER_LAME_SUFFIX_KEY "suffix"
#define ENCODER_LAME_PRESET_KEY "preset"
#define ENCODER_LAME_PRESET_MEDIUM 0
#define ENCODER_LAME_PRESET_STANDARD 1
#define ENCODER_LAME_PRESET_EXTREME 2
#define ENCODER_LAME_PRESET_INSANE 3
#define ENCODER_LAME_PRESET_CUSTOM 4
#define ENCODER_LAME_CBR_KEY "cbr"
#define ENCODER_LAME_BITRATE_KEY "bitrate"
#define ENCODER_LAME_EMBED_COVER_KEY "embed_cover"

#define ENCODER_OPUSENC_SUFFIX_KEY "suffix"
#define ENCODER_OPUSENC_BITRATE_KEY "bitrate"
#define ENCODER_OPUSENC_EMBED_COVER_KEY "embed_cover"

#define ENCODER_FLAC_SUFFIX_KEY "suffix"
#define ENCODER_FLAC_COMPRESSION_KEY "compression"
#define ENCODER_FLAC_EMBED_COVER_KEY "embed_cover"

#define ENCODER_WAVE_SUFFIX_KEY "suffix"

#define ENCODER_CUSTOM_SUFFIX_KEY "suffix"
#define ENCODER_CUSTOM_COMMAND_SCHEME_KEY "command_scheme"

/******************/
/* default values */
/******************/
#define ENCODER_LAME_NAME i18n("MP3")
#define ENCODER_LAME_ENCODER_NAME "LAME"
#define ENCODER_LAME_ICON "audio-mpeg"
#define ENCODER_LAME_SUFFIX "mp3"
#define ENCODER_LAME_EMBED_COVER true

/* preset quality */
#define ENCODER_LAME_PRESET 2
#define ENCODER_LAME_CBR false
#define ENCODER_LAME_BITRATE 224

#define ENCODER_OPUSENC_NAME i18n("Opus")
#define ENCODER_OPUSENC_ENCODER_NAME "OPUSENC"
#define ENCODER_OPUSENC_ICON "audio-x-opus"
#define ENCODER_OPUSENC_SUFFIX "opus"
#define ENCODER_OPUSENC_EMBED_COVER true

/* preset quality */
#define ENCODER_OPUSENC_BITRATE 128

#define ENCODER_FLAC_NAME i18n("FLAC (Lossless)")
#define ENCODER_FLAC_ENCODER_NAME "FLAC"
#define ENCODER_FLAC_ICON "audio-x-flac"
#define ENCODER_FLAC_SUFFIX "flac"

#define ENCODER_FLAC_COMPRESSION 8
#define ENCODER_FLAC_EMBED_COVER true

#define ENCODER_WAVE_NAME i18n("WAVE (Raw Uncompressed)")
#define ENCODER_WAVE_ENCODER_NAME "WAVE"
#define ENCODER_WAVE_ICON "audio-x-wav"
#define ENCODER_WAVE_SUFFIX "wav"

#define ENCODER_CUSTOM_NAME i18n("Custom (External Encoder)")
#define ENCODER_CUSTOM_ENCODER_NAME i18n("Custom")
#define ENCODER_CUSTOM_ICON "audio-x-generic"
#define ENCODER_CUSTOM_SUFFIX ""
#define ENCODER_CUSTOM_COMMAND_SCHEME ""

namespace EncoderAssistant
{
enum Encoder {
    LAME = 0,
    OPUSENC,
    FLAC,
    WAVE,
    CUSTOM,
    NUM
};

const QString name(const Encoder encoder);
const QString encoderName(const Encoder encoder);
const QString icon(const Encoder encoder);

bool available(const Encoder encoder);
const QString version(const Encoder encoder);

// What an encoder plugin is built with, e.g. "LAME" (empty for built-in encoders)
const QString pluginRequirement(const Encoder encoder);
// User visible hint why an encoder is not available (empty if it is)
const QString unavailableReason(const Encoder encoder);

// Encoders with a built-in (plugin) backend: available() returns true only
// when a plugin was registered, version() reports the plugin version.
void setNativeBackend(const Encoder encoder, const QString &version = QString());
bool hasNativeBackend(const Encoder encoder);

Parameters stdParameters(const Encoder encoder);

// Image profiles are restricted to these (WAVE, FLAC)
inline bool lossless(const Encoder encoder)
{
    return encoder == FLAC || encoder == WAVE;
}

};
