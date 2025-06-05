/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "encoderassistant.h"

namespace Audex
{

long makeVersionNumber(int major, int minor, int patch)
{
    long versionNumber = ((major & 0xFF) << 16) | ((minor & 0xFF) << 8) | (patch & 0xFF);
    return versionNumber;
}

const QString EncoderAssistant::name(const EncoderAssistant::Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        return ENCODER_LAME_NAME;
    case EncoderAssistant::OGGENC:
        return ENCODER_OGGENC_NAME;
    case EncoderAssistant::OPUSENC:
        return ENCODER_OPUSENC_NAME;
    case EncoderAssistant::FLAC:
        return ENCODER_FLAC_NAME;
    case EncoderAssistant::FAAC:
        return ENCODER_FAAC_NAME;
    case EncoderAssistant::WAVE:
        return ENCODER_WAVE_NAME;
    case EncoderAssistant::CUSTOM:
        return ENCODER_CUSTOM_NAME;
    default:
        return "";
    }

    return "";
}

const QString EncoderAssistant::encoderName(const Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        return ENCODER_LAME_ENCODER_NAME;
    case EncoderAssistant::OGGENC:
        return ENCODER_OGGENC_ENCODER_NAME;
    case EncoderAssistant::OPUSENC:
        return ENCODER_OPUSENC_ENCODER_NAME;
    case EncoderAssistant::FLAC:
        return ENCODER_FLAC_ENCODER_NAME;
    case EncoderAssistant::FAAC:
        return ENCODER_FAAC_ENCODER_NAME;
    case EncoderAssistant::WAVE:
        return ENCODER_WAVE_ENCODER_NAME;
    case EncoderAssistant::CUSTOM:
        return ENCODER_CUSTOM_ENCODER_NAME;
    default:
        return "";
    }

    return "";
}

const QString EncoderAssistant::icon(const EncoderAssistant::Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        return ENCODER_LAME_ICON;
    case EncoderAssistant::OGGENC:
        return ENCODER_OGGENC_ICON;
    case EncoderAssistant::OPUSENC:
        return ENCODER_OPUSENC_ICON;
    case EncoderAssistant::FLAC:
        return ENCODER_FLAC_ICON;
    case EncoderAssistant::FAAC:
        return ENCODER_FAAC_ICON;
    case EncoderAssistant::WAVE:
        return ENCODER_WAVE_ICON;
    case EncoderAssistant::CUSTOM:
        return ENCODER_CUSTOM_ICON;
    default:
        return "";
    }

    return "";
}

bool EncoderAssistant::available(const EncoderAssistant::Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        return (KProcess::execute(ENCODER_LAME_BIN, QStringList() << ENCODER_LAME_VERSION_PARA) == 0);
    case EncoderAssistant::OGGENC:
        return (KProcess::execute(ENCODER_OGGENC_BIN, QStringList() << ENCODER_OGGENC_VERSION_PARA) == 0);
    case EncoderAssistant::OPUSENC:
        return (KProcess::execute(ENCODER_OPUSENC_BIN, QStringList() << ENCODER_OPUSENC_VERSION_PARA) == 0);
    case EncoderAssistant::FLAC:
        return (KProcess::execute(ENCODER_FLAC_BIN, QStringList() << ENCODER_FLAC_VERSION_PARA) == 0);
    case EncoderAssistant::FAAC:
        return (KProcess::execute(ENCODER_FAAC_BIN, QStringList() << ENCODER_FAAC_VERSION_PARA) == 1);
    case EncoderAssistant::WAVE:
        return (KProcess::execute(ENCODER_WAVE_BIN, QStringList() << ENCODER_WAVE_VERSION_PARA) == 0);
    case EncoderAssistant::CUSTOM:
        return true;
    default:
        return false;
    }

    return false;
}

bool EncoderAssistant::canEmbedCover(const Encoder encoder, int *maxCoverSize)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        if (maxCoverSize)
            *maxCoverSize = ENCODER_LAME_MAX_EMBED_COVER_SIZE;
        return true;
    case EncoderAssistant::OPUSENC:
    case EncoderAssistant::FLAC:
    case EncoderAssistant::FAAC:
        return true;
    case EncoderAssistant::OGGENC:
    case EncoderAssistant::WAVE:
    case EncoderAssistant::CUSTOM:
    default:
        break;
    }

    if (maxCoverSize)
        *maxCoverSize = 0;

    return false;
}

const QString EncoderAssistant::version(const EncoderAssistant::Encoder encoder)
{
    KProcess process;
    process.setOutputChannelMode(KProcess::SeparateChannels);
    process.setReadChannel(KProcess::StandardError);
    switch (encoder) {
    case EncoderAssistant::LAME:
        process.setShellCommand(QString(ENCODER_LAME_BIN) + ' ' + QString(ENCODER_LAME_VERSION_PARA));
        break;
    case EncoderAssistant::OGGENC:
        process.setShellCommand(QString(ENCODER_OGGENC_BIN) + ' ' + QString(ENCODER_OGGENC_VERSION_PARA));
        break;
    case EncoderAssistant::OPUSENC:
        process.setShellCommand(QString(ENCODER_OPUSENC_BIN) + ' ' + QString(ENCODER_OPUSENC_VERSION_PARA));
        break;
    case EncoderAssistant::FLAC:
        process.setShellCommand(QString(ENCODER_FLAC_BIN) + ' ' + QString(ENCODER_FLAC_VERSION_PARA));
        break;
    case EncoderAssistant::FAAC:
        process.setShellCommand(QString(ENCODER_FAAC_BIN) + ' ' + QString(ENCODER_FAAC_VERSION_PARA));
        break;
    case EncoderAssistant::WAVE:
        return "";
    case EncoderAssistant::CUSTOM:
        return "";
    default:
        return "";
    }
    process.start();
    if (!process.waitForFinished())
        return "";
    QByteArray rawoutput = process.readAllStandardError();
    if (rawoutput.size() == 0)
        rawoutput = process.readAllStandardOutput();
    QString output(rawoutput);
    QStringList list = output.trimmed().split('\n');
    if (list.count() == 0)
        return "";
    QStringList words = list[0].split(' ');
    if (words.count() == 0)
        return "";

    switch (encoder) {
    case EncoderAssistant::LAME:
        if ((words.contains(QStringLiteral("version"))) && (words.indexOf(QStringLiteral("version")) + 1 < words.count()))
            return words[words.indexOf(QStringLiteral("version")) + 1];
        if (words.count() < 2)
            return "";
        return words[words.count() - 2];

    case EncoderAssistant::OPUSENC:
        return words[2];

    case EncoderAssistant::OGGENC:

    case EncoderAssistant::FLAC:
        return words.last();

    case EncoderAssistant::FAAC:
        if (list.count() < 2)
            return "";
        words = list[1].split(' ');
        if (words.count() < 2)
            return "";
        if ((words.contains(QStringLiteral("FAAC"))) && (words.indexOf(QStringLiteral("FAAC")) + 1 < words.count()))
            return words[words.indexOf(QStringLiteral("FAAC")) + 1];
        return words[1];

    case EncoderAssistant::WAVE:

    case EncoderAssistant::CUSTOM:

    default:;
    }

    return "";
}

long EncoderAssistant::versionNumber(const EncoderAssistant::Encoder encoder)
{
    QString v = version(encoder);
    long versionNumber = 0;

    switch (encoder) {
    case EncoderAssistant::LAME:

    case EncoderAssistant::OGGENC:

    case EncoderAssistant::OPUSENC:

    case EncoderAssistant::FLAC:

    case EncoderAssistant::FAAC: {
        // At present all encoders seem to use 2 or 3 version number items
        // separated by . so we use the same code for all
        // convert to a number for easy version comparison.
        // Each of the 3 version components must be < 255
        QStringList version = v.split('.');
        int major = 0;
        int minor = 0;
        int patch = 0;

        if (version.count() > 0) {
            major = version[0].toUInt();
            versionNumber = (major & 0xFF) << 16;
        }
        if (version.count() > 1) {
            minor = version[1].toUInt();
            versionNumber = versionNumber | (minor & 0xFF) << 8;
        }
        if (version.count() > 2) {
            patch = version[2].toUInt();
            versionNumber = versionNumber | (patch & 0xFF);
        }
    } break;

    case EncoderAssistant::WAVE:

    case EncoderAssistant::CUSTOM:

    default:;
    }
    return versionNumber;
}

const QString EncoderAssistant::scheme(const EncoderAssistant::Encoder encoder, const Parameters &parameters)
{
    switch (encoder) {
    case EncoderAssistant::LAME: {
        int preset = parameters.value(ENCODER_LAME_PRESET_KEY, ENCODER_LAME_PRESET).toInt();
        bool cbr = parameters.value(ENCODER_LAME_CBR_KEY).toBool();
        int bitrate = parameters.value(ENCODER_LAME_BITRATE_KEY, ENCODER_LAME_BITRATE).toInt();
        bool embed_cover = parameters.value(ENCODER_LAME_EMBED_COVER_KEY).toBool();
        QString cmd = ENCODER_LAME_BIN;
        switch (preset) {
        case ENCODER_LAME_PRESET_MEDIUM:
            cmd += QStringLiteral(" --preset medium");
            break;
        case ENCODER_LAME_PRESET_STANDARD:
            cmd += QStringLiteral(" --preset standard");
            break;
        case ENCODER_LAME_PRESET_EXTREME:
            cmd += QStringLiteral(" --preset extreme");
            break;
        case ENCODER_LAME_PRESET_INSANE:
            cmd += QStringLiteral(" --preset insane");
            break;
        case ENCODER_LAME_PRESET_CUSTOM:
            cmd += QStringLiteral(" --preset") + (cbr ? QStringLiteral(" cbr") : QString()) + QStringLiteral(" %1").arg(bitrate);
            break;
        default:
            cmd += QStringLiteral(" --preset standard");
        }
        QString v = EncoderAssistant::version(EncoderAssistant::LAME);
        if ((v.startsWith(QStringLiteral("3.95"))) || (v.startsWith(QStringLiteral("3.96"))) || (v.startsWith(QStringLiteral("3.97"))))
            cmd += QStringLiteral(" --vbr-new");

        if (embed_cover)
            cmd += QStringLiteral(u" --ti \"${%1}\"").arg(VAR_COVER_FILE);

        cmd += QStringLiteral(
                   u" --add-id3v2 --id3v2-only --ignore-tag-errors --tt \"$%1\" --ta \"$%2\" --tl \"$%3\" --ty \"$%4\" --tn \"$%5/$%6\" --tc \"$%7 / Encoder "
                   u"$%8\" --tg \"$%9\"  --tv TPOS=\"$%10\" \"$%11\" \"$%12\"")
                   .arg(VAR_TRACK_TITLE)
                   .arg(VAR_TRACK_ARTIST)
                   .arg(VAR_ALBUM_TITLE)
                   .arg(VAR_DATE)
                   .arg(VAR_TRACK_NO)
                   .arg(VAR_NO_OF_TRACKS)
                   .arg(VAR_AUDEX)
                   .arg(VAR_ENCODER)
                   .arg(VAR_GENRE)
                   .arg(VAR_CD_NO)
                   .arg(VAR_INPUT_FILE)
                   .arg(VAR_OUTPUT_FILE);

        return cmd;
    }

    case EncoderAssistant::OGGENC: {
        qreal quality = parameters.value(ENCODER_OGGENC_QUALITY_KEY, ENCODER_OGGENC_QUALITY).toReal();
        bool min_bitrate = parameters.value(ENCODER_OGGENC_MINBITRATE_KEY).toBool();
        int min_bitrate_value = parameters.value(ENCODER_OGGENC_MINBITRATE_VALUE_KEY, ENCODER_OGGENC_MINBITRATE_VALUE).toInt();
        bool max_bitrate = parameters.value(ENCODER_OGGENC_MAXBITRATE_KEY).toBool();
        int max_bitrate_value = parameters.value(ENCODER_OGGENC_MAXBITRATE_VALUE_KEY, ENCODER_OGGENC_MAXBITRATE_VALUE).toInt();
        QString cmd = ENCODER_OGGENC_BIN;
        cmd += QStringLiteral(u" -q %1").arg(quality, 0, 'f', 2);

        if (min_bitrate)
            cmd += QStringLiteral(u" -m %1").arg(min_bitrate_value);

        if (max_bitrate)
            cmd += QStringLiteral(u" -M %1").arg(max_bitrate_value);

        cmd += QStringLiteral(
                   u" -c \"Artist=$%1\" -c \"Title=$%2\" -c \"Album=$%3\" -c \"Date=$%4\" -c \"Tracknumber=$%5\" -c \"Genre=$%6\" -c \"Discnumber=$%7\" -o "
                   u"\"$%8\" \"$%9\" ")
                   .arg(VAR_TRACK_ARTIST)
                   .arg(VAR_TRACK_TITLE)
                   .arg(VAR_ALBUM_TITLE)
                   .arg(VAR_DATE)
                   .arg(VAR_TRACK_NO)
                   .arg(VAR_GENRE)
                   .arg(VAR_CD_NO)
                   .arg(VAR_OUTPUT_FILE)
                   .arg(VAR_INPUT_FILE);

        return cmd;
    }

    case EncoderAssistant::OPUSENC: {
        qreal bitrate = parameters.value(ENCODER_OPUSENC_BITRATE_KEY, ENCODER_OPUSENC_BITRATE).toReal();
        bool embed_cover = parameters.value(ENCODER_OPUSENC_EMBED_COVER_KEY).toBool();
        QString cmd = ENCODER_OPUSENC_BIN;
        cmd += QStringLiteral(u" --bitrate %1").arg(bitrate, 0, 'f', 2);

        if (embed_cover)
            cmd += QStringLiteral(u" --picture \"3||||${%1}\"").arg(VAR_COVER_FILE);

        cmd += QStringLiteral(" --artist \"$%1\" --title \"$%2\" --album \"$%3\" --date \"$%4\" --tracknumber \"$%5\" --genre \"$%6\" \"$%7\" \"$%8\" ")
                   .arg(VAR_TRACK_ARTIST)
                   .arg(VAR_TRACK_TITLE)
                   .arg(VAR_ALBUM_TITLE)
                   .arg(VAR_DATE)
                   .arg(VAR_TRACK_NO)
                   .arg(VAR_GENRE)
                   .arg(VAR_INPUT_FILE)
                   .arg(VAR_OUTPUT_FILE);

        return cmd;
    }

    case EncoderAssistant::FLAC: {
        int compression = parameters.value(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION).toInt();
        bool embed_cover = parameters.value(ENCODER_FLAC_EMBED_COVER_KEY).toBool();
        QString cmd = ENCODER_FLAC_BIN;

        if (embed_cover) {
            long versionNumber = EncoderAssistant::versionNumber(EncoderAssistant::FLAC);
            if (versionNumber >= makeVersionNumber(1, 1, 3))
                cmd += QStringLiteral(u" --picture=\"\\|\\|\\|\\|$%1\"").arg(VAR_COVER_FILE);
        }

        cmd += QStringLiteral(u" -%1").arg(compression);

        cmd +=
            QStringLiteral(u" -T Artist=\"$%1\" -T Title=\"$%2\" -T Album=\"$%3\" -T Date=\"$%4\" -T Tracknumber=\"$%5\" -T Genre=\"$%6\" -o \"$%7\" \"$%8\" ")
                .arg(VAR_TRACK_ARTIST)
                .arg(VAR_TRACK_TITLE)
                .arg(VAR_ALBUM_TITLE)
                .arg(VAR_DATE)
                .arg(VAR_TRACK_NO)
                .arg(VAR_GENRE)
                .arg(VAR_OUTPUT_FILE)
                .arg(VAR_INPUT_FILE);

        return cmd;
    }

    case EncoderAssistant::FAAC: {
        int quality = parameters.value(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY).toInt();
        bool embed_cover = parameters.value(ENCODER_FAAC_EMBED_COVER_KEY).toBool();
        QString cmd = ENCODER_FAAC_BIN;
        cmd += QStringLiteral(u" -q %1").arg(quality);

        if (embed_cover)
            cmd += QStringLiteral(u" --cover-art \"${%1}\"").arg(VAR_COVER_FILE);

        cmd += QStringLiteral(
                   u" --artist \"$%1\" --title \"$%2\" --album \"$%3\" --year \"$%4\" --track \"$%5\" --disc \"$%6\" --genre \"$%7\" -o \"$%8\" \"$%9\" ")
                   .arg(VAR_TRACK_ARTIST)
                   .arg(VAR_TRACK_TITLE)
                   .arg(VAR_ALBUM_TITLE)
                   .arg(VAR_DATE)
                   .arg(VAR_TRACK_NO)
                   .arg(VAR_CD_NO)
                   .arg(VAR_GENRE)
                   .arg(VAR_OUTPUT_FILE)
                   .arg(VAR_INPUT_FILE);

        return cmd;
    }

    case EncoderAssistant::WAVE: {
        return QStringLiteral(ENCODER_WAVE_BIN) + QStringLiteral(u" \"$%1\" \"$%2\"").arg(VAR_INPUT_FILE).arg(VAR_OUTPUT_FILE);
    }

    case EncoderAssistant::CUSTOM: {
        return parameters.value(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, ENCODER_CUSTOM_COMMAND_SCHEME).toString();
    }

    default:;
    }

    return "";
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

    case EncoderAssistant::OGGENC:

        parameters.setValue(ENCODER_OGGENC_QUALITY_KEY, ENCODER_OGGENC_QUALITY);
        parameters.setValue(ENCODER_OGGENC_MINBITRATE_KEY, ENCODER_OGGENC_MINBITRATE);
        parameters.setValue(ENCODER_OGGENC_MINBITRATE_VALUE_KEY, ENCODER_OGGENC_MINBITRATE_VALUE);
        parameters.setValue(ENCODER_OGGENC_MAXBITRATE_KEY, ENCODER_OGGENC_MAXBITRATE);
        parameters.setValue(ENCODER_OGGENC_MAXBITRATE_VALUE_KEY, ENCODER_OGGENC_MAXBITRATE_VALUE);

        break;

    case EncoderAssistant::OPUSENC:

        parameters.setValue(ENCODER_OPUSENC_BITRATE_KEY, ENCODER_OPUSENC_BITRATE);
        parameters.setValue(ENCODER_OPUSENC_EMBED_COVER_KEY, ENCODER_OPUSENC_EMBED_COVER);
        break;

    case EncoderAssistant::FLAC:

        parameters.setValue(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION);
        parameters.setValue(ENCODER_FLAC_EMBED_COVER_KEY, ENCODER_FLAC_EMBED_COVER);

        break;

    case EncoderAssistant::FAAC:

        parameters.setValue(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY);
        parameters.setValue(ENCODER_FAAC_EMBED_COVER_KEY, ENCODER_FAAC_EMBED_COVER);
        break;

    case EncoderAssistant::WAVE:

    case EncoderAssistant::CUSTOM:

    default:;
    }

    return parameters;
}

const QMap<int, QString> EncoderAssistant::encoderList()
{
    QMap<int, QString> encoders;

    encoders[(int)EncoderAssistant::LAME] = ENCODER_LAME_NAME;
    encoders[(int)EncoderAssistant::OGGENC] = ENCODER_OGGENC_NAME;
    encoders[(int)EncoderAssistant::FLAC] = ENCODER_FLAC_NAME;
    encoders[(int)EncoderAssistant::FAAC] = ENCODER_FAAC_NAME;
    encoders[(int)EncoderAssistant::WAVE] = ENCODER_WAVE_NAME;
    encoders[(int)EncoderAssistant::CUSTOM] = ENCODER_CUSTOM_NAME;
    encoders[(int)EncoderAssistant::OPUSENC] = ENCODER_OGGENC_NAME;

    return encoders;
}

const QMap<int, QString> EncoderAssistant::availableEncoderNameList()
{
    QMap<int, QString> encoders;

    if (EncoderAssistant::available(EncoderAssistant::LAME))
        encoders[(int)EncoderAssistant::LAME] = ENCODER_LAME_NAME;
    if (EncoderAssistant::available(EncoderAssistant::OGGENC))
        encoders[(int)EncoderAssistant::OGGENC] = ENCODER_OGGENC_NAME;
    if (EncoderAssistant::available(EncoderAssistant::OPUSENC))
        encoders[(int)EncoderAssistant::OPUSENC] = ENCODER_OPUSENC_NAME;
    if (EncoderAssistant::available(EncoderAssistant::FLAC))
        encoders[(int)EncoderAssistant::FLAC] = ENCODER_FLAC_NAME;
    if (EncoderAssistant::available(EncoderAssistant::FAAC))
        encoders[(int)EncoderAssistant::FAAC] = ENCODER_FAAC_NAME;
    if (EncoderAssistant::available(EncoderAssistant::WAVE))
        encoders[(int)EncoderAssistant::WAVE] = ENCODER_WAVE_NAME;
    if (EncoderAssistant::available(EncoderAssistant::CUSTOM))
        encoders[(int)EncoderAssistant::CUSTOM] = ENCODER_CUSTOM_NAME;

    return encoders;
}

const QMap<int, QString> EncoderAssistant::availableEncoderNameListWithVersions()
{
    QMap<int, QString> encoders;

    if (EncoderAssistant::available(EncoderAssistant::LAME))
        encoders[(int)EncoderAssistant::LAME] = ENCODER_LAME_NAME + ' ' + version(LAME);
    if (EncoderAssistant::available(EncoderAssistant::OGGENC))
        encoders[(int)EncoderAssistant::OGGENC] = ENCODER_OGGENC_NAME + ' ' + version(OGGENC);
    if (EncoderAssistant::available(EncoderAssistant::OPUSENC))
        encoders[(int)EncoderAssistant::OPUSENC] = ENCODER_OPUSENC_NAME + ' ' + version(OPUSENC);
    if (EncoderAssistant::available(EncoderAssistant::FLAC))
        encoders[(int)EncoderAssistant::FLAC] = ENCODER_FLAC_NAME + ' ' + version(FLAC);
    if (EncoderAssistant::available(EncoderAssistant::FAAC))
        encoders[(int)EncoderAssistant::FAAC] = ENCODER_FAAC_NAME + ' ' + version(FAAC);
    if (EncoderAssistant::available(EncoderAssistant::WAVE))
        encoders[(int)EncoderAssistant::WAVE] = ENCODER_WAVE_NAME + ' ' + version(WAVE);
    if (EncoderAssistant::available(EncoderAssistant::CUSTOM))
        encoders[(int)EncoderAssistant::CUSTOM] = ENCODER_CUSTOM_NAME + ' ' + version(CUSTOM);

    return encoders;
}

}
