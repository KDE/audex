/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ENCODERASSISTANT_H
#define ENCODERASSISTANT_H

#include <QMap>
#include <QString>
#include <QStringList>

#include <KLocalizedString>
#include <KProcess>

#include "utils/parameters.h"
#include "utils/patternparser.h"

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

#define ENCODER_OGGENC_SUFFIX_KEY "suffix"
#define ENCODER_OGGENC_QUALITY_KEY "quality"
#define ENCODER_OGGENC_MINBITRATE_KEY "minbitrate"
#define ENCODER_OGGENC_MINBITRATE_VALUE_KEY "minbitrate_value"
#define ENCODER_OGGENC_MAXBITRATE_KEY "maxbitrate"
#define ENCODER_OGGENC_MAXBITRATE_VALUE_KEY "maxbitrate_value"

#define ENCODER_OPUSENC_SUFFIX_KEY "suffix"
#define ENCODER_OPUSENC_BITRATE_KEY "bitrate"
#define ENCODER_OPUSENC_EMBED_COVER_KEY "embed_cover"

#define ENCODER_FLAC_SUFFIX_KEY "suffix"
#define ENCODER_FLAC_COMPRESSION_KEY "compression"
#define ENCODER_FLAC_EMBED_COVER_KEY "embed_cover"

#define ENCODER_FAAC_SUFFIX_KEY "suffix"
#define ENCODER_FAAC_QUALITY_KEY "quality"
#define ENCODER_FAAC_EMBED_COVER_KEY "embed_cover"

#define ENCODER_WAVE_SUFFIX_KEY "suffix"

#define ENCODER_CUSTOM_SUFFIX_KEY "suffix"
#define ENCODER_CUSTOM_COMMAND_PATTERN_KEY "command_pattern"

/******************/
/* default values */
/******************/
#define ENCODER_LAME_NAME i18n("MP3")
#define ENCODER_LAME_ENCODER_NAME "LAME"
#define ENCODER_LAME_ICON "audio-mpeg"
#define ENCODER_LAME_BIN "lame"
#define ENCODER_LAME_VERSION_PARA "--version"
#define ENCODER_LAME_SUFFIX "mp3"
#define ENCODER_LAME_MAX_EMBED_COVER_SIZE 128 * 1024
#define ENCODER_LAME_EMBED_COVER true

/* preset quality */
#define ENCODER_LAME_PRESET 2
#define ENCODER_LAME_CBR false
#define ENCODER_LAME_BITRATE 224

#define ENCODER_OGGENC_NAME i18n("Ogg Vorbis")
#define ENCODER_OGGENC_ENCODER_NAME "OGGENC"
#define ENCODER_OGGENC_ICON "audio-x-vorbis+ogg"
#define ENCODER_OGGENC_BIN "oggenc"
#define ENCODER_OGGENC_VERSION_PARA "--version"
#define ENCODER_OGGENC_SUFFIX "ogg"

/* preset quality */
#define ENCODER_OGGENC_QUALITY 5
#define ENCODER_OGGENC_MINBITRATE false
#define ENCODER_OGGENC_MINBITRATE_VALUE 80
#define ENCODER_OGGENC_MAXBITRATE false
#define ENCODER_OGGENC_MAXBITRATE_VALUE 320

#define ENCODER_OPUSENC_NAME i18n("Opus")
#define ENCODER_OPUSENC_ENCODER_NAME "OPUSENC"
#define ENCODER_OPUSENC_ICON "audio-x-opus"
#define ENCODER_OPUSENC_BIN "opusenc"
#define ENCODER_OPUSENC_VERSION_PARA "--version"
#define ENCODER_OPUSENC_SUFFIX "opus"
#define ENCODER_OPUSENC_EMBED_COVER true

/* preset quality */
#define ENCODER_OPUSENC_BITRATE 128

#define ENCODER_FLAC_NAME i18n("FLAC (Lossless)")
#define ENCODER_FLAC_ENCODER_NAME "FLAC"
#define ENCODER_FLAC_ICON "audio-x-flac"
#define ENCODER_FLAC_BIN "flac"
#define ENCODER_FLAC_VERSION_PARA "-v"
#define ENCODER_FLAC_SUFFIX "flac"

#define ENCODER_FLAC_COMPRESSION 5
#define ENCODER_FLAC_EMBED_COVER true

#define ENCODER_FAAC_NAME i18n("MP4 (AAC)")
#define ENCODER_FAAC_ENCODER_NAME "FAAC"
#define ENCODER_FAAC_ICON "audio-mp4"
#define ENCODER_FAAC_BIN "faac"
#define ENCODER_FAAC_VERSION_PARA "--help"
#define ENCODER_FAAC_SUFFIX "mp4"
#define ENCODER_FAAC_EMBED_COVER true

/* preset quality */
#define ENCODER_FAAC_QUALITY 160

#define ENCODER_WAVE_NAME i18n("WAVE (Raw Uncompressed)")
#define ENCODER_WAVE_ENCODER_NAME "WAVE"
#define ENCODER_WAVE_ICON "audio-x-wav"
#define ENCODER_WAVE_BIN "mv"
#define ENCODER_WAVE_VERSION_PARA "--help"
#define ENCODER_WAVE_SUFFIX "wav"

#define ENCODER_CUSTOM_NAME i18n("Custom")
#define ENCODER_CUSTOM_ENCODER_NAME i18n("Custom")
#define ENCODER_CUSTOM_ICON "audio-x-generic"
#define ENCODER_CUSTOM_BIN ""
#define ENCODER_CUSTOM_VERSION_PARA ""
#define ENCODER_CUSTOM_SUFFIX ""
#define ENCODER_CUSTOM_COMMAND_PATTERN ""

#define ENCODER_NUM 6

namespace EncoderAssistant
{
enum Encoder { LAME = 0, OGGENC, OPUSENC, FLAC, FAAC, WAVE, CUSTOM, NUM };

const QString name(const Encoder encoder);
const QString encoderName(const Encoder encoder);
const QString icon(const Encoder encoder);

bool available(const Encoder encoder);
bool canEmbedCover(const Encoder encoder, int *maxCoverSize = nullptr);
const QString version(const Encoder encoder);
long versionNumber(const Encoder encoder);
const QString pattern(const Encoder encoder, const Parameters &parameters);

Parameters stdParameters(const Encoder encoder);

const QMap<int, QString> encoderList();
const QMap<int, QString> availableEncoderNameList();
const QMap<int, QString> availableEncoderNameListWithVersions();

};

#endif
