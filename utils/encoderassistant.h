/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2013 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ENCODERASSISTANT_H
#define ENCODERASSISTANT_H

#include <QString>
#include <QStringList>
#include <QMap>

#include <KDebug>
#include <KLocale>
#include <KProcess>

#include "utils/parameters.h"
#include "utils/patternparser.h"

#define ENCODER_LAME_SUFFIX_KEY			"suffix"
#define ENCODER_LAME_PRESET_KEY			"preset"
#define ENCODER_LAME_PRESET_MEDIUM		0
#define ENCODER_LAME_PRESET_STANDARD		1
#define ENCODER_LAME_PRESET_EXTREME		2
#define ENCODER_LAME_PRESET_INSANE		3
#define ENCODER_LAME_PRESET_CUSTOM		4
#define ENCODER_LAME_CBR_KEY			"cbr"
#define ENCODER_LAME_BITRATE_KEY		"bitrate"
#define ENCODER_LAME_EMBED_COVER_KEY		"embed_cover"

#define ENCODER_OGGENC_SUFFIX_KEY		"suffix"
#define ENCODER_OGGENC_QUALITY_KEY		"quality"
#define ENCODER_OGGENC_MINBITRATE_KEY		"minbitrate"
#define ENCODER_OGGENC_MINBITRATE_VALUE_KEY	"minbitrate_value"
#define ENCODER_OGGENC_MAXBITRATE_KEY		"maxbitrate"
#define ENCODER_OGGENC_MAXBITRATE_VALUE_KEY	"maxbitrate_value"

#define ENCODER_FLAC_SUFFIX_KEY			"suffix"
#define ENCODER_FLAC_COMPRESSION_KEY		"compression"

#define ENCODER_FAAC_SUFFIX_KEY			"suffix"
#define ENCODER_FAAC_QUALITY_KEY		"quality"

#define ENCODER_WAVE_SUFFIX_KEY			"suffix"

#define ENCODER_CUSTOM_SUFFIX_KEY		"suffix"
#define ENCODER_CUSTOM_COMMAND_PATTERN_KEY	"command_pattern"


/******************/
/* default values */
/******************/
#define ENCODER_LAME_NAME			i18n("MP3")
#define ENCODER_LAME_ENCODER_NAME		"LAME"
#define ENCODER_LAME_ICON			"audio-mpeg"
#define ENCODER_LAME_BIN			"lame"
#define ENCODER_LAME_VERSION_PARA		"--version"
#define ENCODER_LAME_SUFFIX			"mp3"
#define ENCODER_LAME_MAX_EMBED_COVER_SIZE	128*1024

/* preset normal quality */
#define ENCODER_LAME_PRESET			1
#define ENCODER_LAME_CBR			"false"
#define ENCODER_LAME_BITRATE			192
#define ENCODER_LAME_EMBED_COVER		"false"

/* preset mobile quality */
#define ENCODER_LAME_PRESET_M			4
#define ENCODER_LAME_CBR_M			"false"
#define ENCODER_LAME_BITRATE_M			128
#define ENCODER_LAME_EMBED_COVER_M		"false"

/* preset extreme quality */
#define ENCODER_LAME_PRESET_X			2
#define ENCODER_LAME_CBR_X			"false"
#define ENCODER_LAME_BITRATE_X			192
#define ENCODER_LAME_EMBED_COVER_X		"false"

/* tag helper for lame */
#define ENCODER_LAME_HELPER_TAG			"eyeD3"
#define ENCODER_LAME_HELPER_TAG_VERSION_PARA	"--version"



#define ENCODER_OGGENC_NAME			i18n("Ogg Vorbis")
#define ENCODER_OGGENC_ENCODER_NAME		"OGGENC"
#define ENCODER_OGGENC_ICON			"audio-x-vorbis+ogg"
#define ENCODER_OGGENC_BIN			"oggenc"
#define ENCODER_OGGENC_VERSION_PARA		"--version"
#define ENCODER_OGGENC_SUFFIX			"ogg"

/* preset normal quality */
#define ENCODER_OGGENC_QUALITY			4
#define ENCODER_OGGENC_MINBITRATE		FALSE
#define ENCODER_OGGENC_MINBITRATE_VALUE		80
#define ENCODER_OGGENC_MAXBITRATE		FALSE
#define ENCODER_OGGENC_MAXBITRATE_VALUE		256

/* preset mobile quality */
#define ENCODER_OGGENC_QUALITY_M		2
#define ENCODER_OGGENC_MINBITRATE_M		FALSE
#define ENCODER_OGGENC_MINBITRATE_VALUE_M	80
#define ENCODER_OGGENC_MAXBITRATE_M		TRUE
#define ENCODER_OGGENC_MAXBITRATE_VALUE_M	224

/* preset extreme quality */
#define ENCODER_OGGENC_QUALITY_X		6
#define ENCODER_OGGENC_MINBITRATE_X		FALSE
#define ENCODER_OGGENC_MINBITRATE_VALUE_X	80
#define ENCODER_OGGENC_MAXBITRATE_X		FALSE
#define ENCODER_OGGENC_MAXBITRATE_VALUE_X	256



#define ENCODER_FLAC_NAME			i18n("FLAC (Lossless)")
#define ENCODER_FLAC_ENCODER_NAME		"FLAC"
#define ENCODER_FLAC_ICON			"audio-x-flac"
#define ENCODER_FLAC_BIN			"flac"
#define ENCODER_FLAC_VERSION_PARA		"-v"
#define ENCODER_FLAC_SUFFIX			"flac"

#define ENCODER_FLAC_COMPRESSION		5



#define ENCODER_FAAC_NAME			i18n("MP4 (AAC)")
#define ENCODER_FAAC_ENCODER_NAME		"FAAC"
#define ENCODER_FAAC_ICON			"audio-mp4"
#define ENCODER_FAAC_BIN			"faac"
#define ENCODER_FAAC_VERSION_PARA		"--help"
#define ENCODER_FAAC_SUFFIX			"mp4"

/* preset normal quality */
#define ENCODER_FAAC_QUALITY			150

/* preset mobile quality */
#define ENCODER_FAAC_QUALITY_M			110

/* preset extreme quality */
#define ENCODER_FAAC_QUALITY_X			300



#define ENCODER_WAVE_NAME			i18n("WAVE (Raw Uncompressed)")
#define ENCODER_WAVE_ENCODER_NAME		"WAVE"
#define ENCODER_WAVE_ICON			"audio-x-wav"
#define ENCODER_WAVE_BIN			"mv"
#define ENCODER_WAVE_VERSION_PARA		"--help"
#define ENCODER_WAVE_SUFFIX			"wav"



#define ENCODER_CUSTOM_NAME			i18n("Custom")
#define ENCODER_CUSTOM_ENCODER_NAME		i18n("Custom")
#define ENCODER_CUSTOM_ICON			"audio-x-generic"
#define ENCODER_CUSTOM_BIN			""
#define ENCODER_CUSTOM_VERSION_PARA		""
#define ENCODER_CUSTOM_SUFFIX			""
#define ENCODER_CUSTOM_COMMAND_PATTERN		""



#define ENCODER_NUM				6


namespace EncoderAssistant {

  enum Encoder {
    LAME = 0,
    OGGENC,
    FLAC,
    FAAC,
    WAVE,
    CUSTOM,
    NUM
  };

  const QString name(const Encoder encoder);
  const QString encoderName(const Encoder encoder);
  const QString icon(const Encoder encoder);

  bool available(const Encoder encoder);
  bool canEmbedCover(const Encoder encoder, int *maxCoverSize = NULL);
  const QString version(const Encoder encoder);
  const QString pattern(const Encoder encoder, const Parameters& parameters);

  enum Quality {
    MOBILE = 0,
    NORMAL,
    EXTREME
  };

  Parameters stdParameters(const Encoder encoder, const Quality quality);

  const QMap<int,QString> encoderList();
  const QMap<int,QString> availableEncoderNameList();
  const QMap<int,QString> availableEncoderNameListWithVersions();

};

#endif
