/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "encoderassistant.h"

const QString EncoderAssistant::name(const EncoderAssistant::Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        return ENCODER_LAME_NAME;
    case EncoderAssistant::OPUSENC:
        return ENCODER_OPUSENC_NAME;
    case EncoderAssistant::FLAC:
        return ENCODER_FLAC_NAME;
    case EncoderAssistant::WAVE:
        return ENCODER_WAVE_NAME;
    case EncoderAssistant::CUSTOM:
        return ENCODER_CUSTOM_NAME;
    default:
        return "";
    }
}

const QString EncoderAssistant::encoderName(const Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        return ENCODER_LAME_ENCODER_NAME;
    case EncoderAssistant::OPUSENC:
        return ENCODER_OPUSENC_ENCODER_NAME;
    case EncoderAssistant::FLAC:
        return ENCODER_FLAC_ENCODER_NAME;
    case EncoderAssistant::WAVE:
        return ENCODER_WAVE_ENCODER_NAME;
    case EncoderAssistant::CUSTOM:
        return ENCODER_CUSTOM_ENCODER_NAME;
    default:
        return "";
    }
}

const QString EncoderAssistant::icon(const EncoderAssistant::Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        return ENCODER_LAME_ICON;
    case EncoderAssistant::OPUSENC:
        return ENCODER_OPUSENC_ICON;
    case EncoderAssistant::FLAC:
        return ENCODER_FLAC_ICON;
    case EncoderAssistant::WAVE:
        return ENCODER_WAVE_ICON;
    case EncoderAssistant::CUSTOM:
        return ENCODER_CUSTOM_ICON;
    default:
        return "";
    }
}

static QHash<int, QString> g_nativeBackends;

void EncoderAssistant::setNativeBackend(const EncoderAssistant::Encoder encoder, const QString &version)
{
    g_nativeBackends.insert((int)encoder, version);
}

bool EncoderAssistant::hasNativeBackend(const EncoderAssistant::Encoder encoder)
{
    return g_nativeBackends.contains((int)encoder);
}

bool EncoderAssistant::available(const EncoderAssistant::Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
    case EncoderAssistant::OPUSENC:
    case EncoderAssistant::FLAC:
        // native encoder plugins only (no external commands)
        return hasNativeBackend(encoder);
    case EncoderAssistant::WAVE:
        return true; // built into the engine
    case EncoderAssistant::CUSTOM:
        return true;
    default:
        return false;
    }
}

const QString EncoderAssistant::version(const EncoderAssistant::Encoder encoder)
{
    // native plugins report the version of the linked codec library
    return g_nativeBackends.value((int)encoder);
}

const QString EncoderAssistant::pluginRequirement(const EncoderAssistant::Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        return QStringLiteral("LAME");
    case EncoderAssistant::OPUSENC:
        return QStringLiteral("libopusenc");
    case EncoderAssistant::FLAC:
        return QStringLiteral("libFLAC");
    default:
        return QString();
    }
}

const QString EncoderAssistant::unavailableReason(const EncoderAssistant::Encoder encoder)
{
    if (available(encoder))
        return QString();
    switch (encoder) {
    case EncoderAssistant::LAME:
        return i18n("The MP3 encoder plugin is not installed (it requires %1).", pluginRequirement(encoder));
    case EncoderAssistant::OPUSENC:
        return i18n("The Opus encoder plugin is not installed (it requires %1).", pluginRequirement(encoder));
    case EncoderAssistant::FLAC:
        return i18n("The FLAC encoder plugin is not installed (it requires %1).", pluginRequirement(encoder));
    default:
        return i18n("The encoder is not available.");
    }
}

Parameters EncoderAssistant::stdParameters(const Encoder encoder)
{
    Parameters parameters;

    switch (encoder) {
    case EncoderAssistant::LAME:

        parameters.setValue(ENCODER_LAME_PRESET_KEY, ENCODER_LAME_PRESET);
        parameters.setValue(ENCODER_LAME_BITRATE_KEY, ENCODER_LAME_BITRATE);
        parameters.setValue(ENCODER_LAME_EMBED_COVER_KEY, ENCODER_LAME_EMBED_COVER);

        break;

    case EncoderAssistant::OPUSENC:

        parameters.setValue(ENCODER_OPUSENC_BITRATE_KEY, ENCODER_OPUSENC_BITRATE);
        parameters.setValue(ENCODER_OPUSENC_EMBED_COVER_KEY, ENCODER_OPUSENC_EMBED_COVER);
        break;

    case EncoderAssistant::FLAC:

        parameters.setValue(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION);
        parameters.setValue(ENCODER_FLAC_EMBED_COVER_KEY, ENCODER_FLAC_EMBED_COVER);

        break;

    case EncoderAssistant::CUSTOM:

        parameters.setValue(ENCODER_CUSTOM_EMBED_COVER_KEY, ENCODER_CUSTOM_EMBED_COVER);

        break;

    case EncoderAssistant::WAVE:

    default:;
    }

    return parameters;
}
