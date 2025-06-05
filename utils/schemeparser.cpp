/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "schemeparser.h"

#include "audex-version.h"

#define IS_TRUE(val) (val.typeId() == QMetaType::Bool && val.toBool())
#define IS_INT(val)                                                                                                                                            \
    (val.typeId() == QMetaType::Int || val.typeId() == QMetaType::UInt || val.typeId() == QMetaType::LongLong || val.typeId() == QMetaType::ULongLong)
#define IS_DATETIME(val) (val.typeId() == QMetaType::QDateTime || val.typeId() == QMetaType::QDate || val.typeId() == QMetaType::QTime)

namespace Audex
{

const QString SchemeParser::parseScheme(const QString &scheme, const Placeholders &placeholders, PlaceholdersParameters *placeholders_parameters)
{
    p_error_string.clear();

    QString result;

    bool region_placeholdername = false;
    int index_placeholdername = 0;
    QString placeholdername;

    bool region_in_braces = false;

    bool region_placeholdername_in_braces = false;
    bool region_parameters_in_braces = false;

    QString parameters_string;

    int index = 0;
    while (index < scheme.length()) {
        QChar c = scheme[index];

        if (region_in_braces) {
            if (region_placeholdername_in_braces) {
                if (c.isLetterOrNumber() || c == QChar('_')) {
                    placeholdername.append(c);
                } else if (c == '}') {
                    if (placeholders.contains(placeholdername)) {
                        result.append(placeholders[placeholdername].toString());
                    }
                    if (placeholders_parameters)
                        placeholders_parameters->insert(placeholdername, Parameters());
                    region_placeholdername_in_braces = false;
                    region_in_braces = false;
                    placeholdername.clear();
                } else if (c.isSpace()) {
                    if (!placeholdername.isEmpty()) {
                        region_placeholdername_in_braces = false;
                        region_parameters_in_braces = true;
                    }
                } else {
                    p_error_string = i18n("Illegal character found at index %1: %2").arg(index).arg(c);
                    return QString();
                }

            } else if (region_parameters_in_braces) {
                if (c == '}') {
                    Parameters parameters(parameters_string, QChar(' '));

                    if (parameters.error()) {
                        p_error_string = parameters.errorString();
                        return QString();
                    }

                    if (placeholders_parameters)
                        placeholders_parameters->insert(placeholdername, parameters);

                    if (placeholders.contains(placeholdername)) {
                        QVariant value = placeholders[placeholdername];

                        KeyList keylist = parameters.keys();
                        for (int i = 0; i < keylist.size(); ++i) {
                            const QString key = keylist.at(i);

                            if (key == "uppercase" && IS_TRUE(parameters.value("uppercase")))
                                value = value.toString().toUpper();

                            if (key == "lowercase" && IS_TRUE(parameters.value("lowercase")))
                                value = value.toString().toLower();

                            if (key == "left" && IS_INT(parameters.value("left"))) {
                                int left = parameters.value("left").toInt();
                                if (left > 0)
                                    value = value.toString().left(left);
                            }

                            if (key == "underscores" && IS_TRUE(parameters.value("underscores")))
                                value = replaceSpacesWithUnderscores(value.toString());

                            if (key == "fat32compatible" && IS_TRUE(parameters.value("fat32compatible")))
                                value = makeFAT32FilenameCompatible(value.toString());

                            if ((key == "replace_char_list" && IS_TRUE(parameters.value("replace_char_list")))
                                || (key == "replace_chars" && IS_TRUE(parameters.value("replace_chars")))) {
                                if (parameters.contains("replace_char_list_from") && parameters.contains("replace_char_list_to")
                                    && parameters.value("replace_char_list_from").toString().length()
                                        == parameters.value("replace_char_list_to").toString().length())
                                    value = QVariant(replaceCharList(parameters.value("replace_char_list_from").toString(),
                                                                     parameters.value("replace_char_list_to").toString(),
                                                                     value.toString()));
                                else
                                    p_error_string = i18n(
                                                         "replace_char_list parameter needs two more parameters replace_char_list_from and "
                                                         "replace_char_list_to with equal size.")
                                                         .arg(index)
                                                         .arg(c);
                            }

                            if (key == "length" && IS_INT(value) && IS_INT(parameters.value("length"))) {
                                int number = value.toInt();
                                int l = parameters.value("length").toInt();
                                QChar fc = '0';
                                if (parameters.contains("fillchar") && !parameters.value("fillchar").toString().isEmpty())
                                    fc = parameters.value("fillchar").toString().at(0);
                                value = QString("%1").arg(number, l, 10, fc);
                            }

                            if (key == "iec" && IS_INT(value)) {
                                QChar iec;
                                if (!parameters.value("iec").toString().isEmpty())
                                    iec = parameters.value("iec").toString().at(0).toLower();
                                if ((iec != 'b') && (iec != 'k') && (iec != 'm') && (iec != 'g'))
                                    iec = 'm';
                                int p = 2;
                                if (parameters.contains("precision"))
                                    p = parameters.value("precision").toInt();
                                if (p < 1)
                                    p = 2;
                                if (iec == 'b')
                                    value = QString("%1").arg(value.toLongLong() / 1.0f, 0, 'f', p);
                                else if (iec == 'k')
                                    value = QString("%1").arg(value.toLongLong() / 1024.0f, 0, 'f', p);
                                else if (iec == 'm')
                                    value = QString("%1").arg(value.toLongLong() / (1024.0f * 1024.0f), 0, 'f', p);
                                else if (iec == 'g')
                                    value = QString("%1").arg(value.toLongLong() / (1024.0f * 1024.0f * 1024.0f), 0, 'f', p);
                            }

                            if (key == "format_datetime" && IS_DATETIME(value)) {
                                QString locale_name;
                                if (!parameters.value("locale").toString().isEmpty())
                                    locale_name = parameters.value("locale").toString();
                                QString format;
                                if (!parameters.value("format_datetime").toString().isEmpty())
                                    format = parameters.value("format_datetime").toString();
                                if (!parameters.value("format").toString().isEmpty())
                                    format = parameters.value("format").toString();
                                if (format.isEmpty()) {
                                    if (!locale_name.isEmpty()) {
                                        QLocale locale(locale_name);
                                        value = locale.toString(value.toDateTime());
                                    } else {
                                        value = value.toDateTime().toString();
                                    }
                                } else {
                                    if (!locale_name.isEmpty()) {
                                        QLocale locale(locale_name);
                                        value = locale.toString(value.toDateTime(), format);
                                    } else {
                                        value = value.toDateTime().toString(format);
                                    }
                                }
                            }

                            if (key == "pre" || key == "preparam") {
                                QString pre;
                                if (key == "preparam")
                                    pre = parameters.value("preparam").toString();
                                else
                                    pre = parameters.value("pre").toString();
                                value = QString("%1%2").arg(pre).arg(value.toString());
                            }

                            if (key == "post" || key == "postparam") {
                                QString post;
                                if (key == "postparam")
                                    post = parameters.value("postparam").toString();
                                else
                                    post = parameters.value("post").toString();
                                value = QString("%1%2").arg(value.toString()).arg(post);
                            }
                        }

                        result.append(value.toString());

                        region_parameters_in_braces = false;
                        region_in_braces = false;
                        placeholdername.clear();

                        parameters_string.clear();
                    }

                } else {
                    parameters_string.append(c);
                }
            }

        } else if (region_placeholdername) {
            if (c == '{' && index_placeholdername == 0) {
                region_in_braces = true;
                region_placeholdername_in_braces = true;
                region_placeholdername = false;
                placeholdername.clear();
                index_placeholdername = 0;
            } else {
                if (!c.isLetterOrNumber() && c != QChar('_')) {
                    if (placeholders.contains(placeholdername)) {
                        result.append(placeholders[placeholdername].toString());
                    }
                    if (placeholders_parameters)
                        placeholders_parameters->insert(placeholdername, Parameters());
                    region_placeholdername = false;
                    placeholdername.clear();
                    index_placeholdername = 0;
                    result.append(c);
                } else {
                    placeholdername.append(c);
                    ++index_placeholdername;
                }
            }

        } else {
            switch (c.unicode()) {
            case '$':
                region_placeholdername = true;
                break;
            case '\\':
                if (index < scheme.length() - 1 && scheme[index + 1] == '$') {
                    result.append('$');
                    ++index;
                }
                break;
            default:
                result.append(c);
            }
        }

        ++index;
    }

    if (placeholders.contains(placeholdername)) {
        result.append(placeholders[placeholdername].toString());
    }

    if (placeholders_parameters && !placeholdername.isEmpty())
        placeholders_parameters->insert(placeholdername, Parameters());

    if (region_in_braces) {
        p_error_string = i18n("Unclosed brace.");
        return QString();
    }

    return result;
}

const QString SchemeParser::parsePerTrackFilenameScheme(const QString &scheme,
                                                        const int trackno,
                                                        const int cdno,
                                                        const int trackoffset,
                                                        const int nooftracks,
                                                        const QString &artist,
                                                        const QString &title,
                                                        const QString &tartist,
                                                        const QString &ttitle,
                                                        const QString &date,
                                                        const QString &genre,
                                                        const QString &isrc,
                                                        const QString &suffix,
                                                        const bool fat32_compatible,
                                                        const bool replace_spaces_with_underscores,
                                                        const bool two_digits_tracknum)
{
    Placeholders placeholders;

    QString fartist = artist;
    QString ftitle = title;
    QString ftartist = tartist;
    QString fttitle = ttitle;
    QString fdate = date;
    QString fgenre = genre;
    QString fisrc = isrc;
    QString fsuffix = suffix;

    if (fat32_compatible) {
        fartist = makeFAT32FilenameCompatible(fartist);
        ftitle = makeFAT32FilenameCompatible(ftitle);
        ftartist = makeFAT32FilenameCompatible(ftartist);
        fttitle = makeFAT32FilenameCompatible(fttitle);
        fdate = makeFAT32FilenameCompatible(fdate);
        fgenre = makeFAT32FilenameCompatible(fgenre);
        fisrc = makeFAT32FilenameCompatible(fisrc);
        fsuffix = makeFAT32FilenameCompatible(fsuffix);
    }

    if (replace_spaces_with_underscores) {
        fartist = replaceSpacesWithUnderscores(fartist);
        ftitle = replaceSpacesWithUnderscores(ftitle);
        ftartist = replaceSpacesWithUnderscores(ftartist);
        fttitle = replaceSpacesWithUnderscores(fttitle);
        fdate = replaceSpacesWithUnderscores(fdate);
        fgenre = replaceSpacesWithUnderscores(fgenre);
        fisrc = replaceSpacesWithUnderscores(fisrc);
        fsuffix = replaceSpacesWithUnderscores(fsuffix);
    }

    placeholders.insert(VAR_ALBUM_ARTIST, fartist);
    placeholders.insert(VAR_ALBUM_TITLE, ftitle);
    placeholders.insert(VAR_TRACK_ARTIST, ftartist);
    placeholders.insert(VAR_TRACK_TITLE, fttitle);
    placeholders.insert(VAR_DATE, fdate);
    placeholders.insert(VAR_GENRE, fgenre);
    placeholders.insert(VAR_ISRC, fisrc);

    placeholders.insert(VAR_SUFFIX, fsuffix);

    int tn = trackno;
    if (trackoffset > 1)
        tn += trackoffset;

    if (two_digits_tracknum)
        placeholders.insert(VAR_TRACK_NO, QString("%1").arg(tn, 2, 10, QChar('0')));
    else
        placeholders.insert(VAR_TRACK_NO, tn);

    placeholders.insert(VAR_CD_NO, cdno);
    placeholders.insert(VAR_NO_OF_TRACKS, nooftracks);

    return parseScheme(scheme, placeholders);
}

const QString SchemeParser::parsePerTrackCommandScheme(const QString &scheme,
                                                       const QString &input,
                                                       const QString &output,
                                                       const int trackno,
                                                       const int cdno,
                                                       const int trackoffset,
                                                       const int nooftracks,
                                                       const QString &artist,
                                                       const QString &title,
                                                       const QString &tartist,
                                                       const QString &ttitle,
                                                       const QString &date,
                                                       const QString &genre,
                                                       const QString &isrc,
                                                       const QString &suffix,
                                                       const QImage &cover,
                                                       const QString &tmppath,
                                                       const QString &encoder,
                                                       const bool demomode)
{
    Placeholders placeholders;

    placeholders.insert(VAR_INPUT_FILE, input);
    placeholders.insert(VAR_OUTPUT_FILE, output);

    placeholders.insert(VAR_ALBUM_ARTIST, mask_inner_quotes(artist));
    placeholders.insert(VAR_ALBUM_TITLE, mask_inner_quotes(title));
    placeholders.insert(VAR_TRACK_ARTIST, mask_inner_quotes(tartist));
    placeholders.insert(VAR_TRACK_TITLE, mask_inner_quotes(ttitle));
    placeholders.insert(VAR_DATE, mask_inner_quotes(date));
    placeholders.insert(VAR_GENRE, mask_inner_quotes(genre));
    placeholders.insert(VAR_ISRC, isrc);
    placeholders.insert(VAR_ENCODER, mask_inner_quotes(encoder));

    placeholders.insert(VAR_AUDEX, AUDEX_VERSION_STRING);

    placeholders.insert(VAR_SUFFIX, suffix);

    int tn = trackno;
    if (trackoffset > 1)
        tn += trackoffset;
    placeholders.insert(VAR_TRACK_NO, tn);

    placeholders.insert(VAR_CD_NO, cdno);
    placeholders.insert(VAR_NO_OF_TRACKS, nooftracks);

    PlaceholdersParameters placeholders_found;

    // Just parse now only to check if cover placeholder is used and retrieve cover parameters - parse again later on:
    QString result = parseScheme(scheme, placeholders, &placeholders_found);

    if (placeholders_found.contains(VAR_COVER_FILE)) {
        Parameters cover_parameters = placeholders_found.value(VAR_COVER_FILE, Parameters());

        QString format = STANDARD_EMBED_COVER_FORMAT;
        if (cover_parameters.contains("formatimage") && !cover_parameters.value("formatimage").toString().isEmpty())
            format = cover_parameters.value("formatimage").toString();
        if (cover_parameters.contains("format") && !cover_parameters.value("format").toString().isEmpty())
            format = cover_parameters.value("format").toString();

        if (!QImageReader::supportedImageFormats().contains(format.toLatin1().toUpper()))
            format = STANDARD_EMBED_COVER_FORMAT;

        QString cover_filepath;
        bool success = false;
        if (demomode) {
            cover_filepath = tmppath + "/cover-embedded." + format.toLower();
            success = true;

        } else {
            int x = -1;
            int y = -1;
            bool ok;
            if (cover_parameters.contains("x")) {
                // when *ok is false, QString::toInt() often return 0
                x = cover_parameters.value("x").toInt(&ok);
                if (!ok)
                    x = -1;
            }
            if (cover_parameters.contains("y")) {
                // when *ok is false, QString::toInt() often return 0
                x = cover_parameters.value("y").toInt(&ok);
                if (!ok)
                    y = -1;
            }

            cover_filepath = tmppath + "/cover-embedded." + format.toLower();

            if (!QFileInfo::exists(cover_filepath)) {
                if (cover.isNull()) {
                    success = QImage(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString("audex/images/cdcase_wo_latches.png")))
                                  .scaledToWidth(STANDARD_EMBED_COVER_WIDTH, Qt::SmoothTransformation)
                                  .save(cover_filepath, format.toLatin1());
                } else {
                    if (x != -1 && y != -1)
                        success = cover.scaled(QSize(x, y), Qt::IgnoreAspectRatio, Qt::SmoothTransformation).save(cover_filepath);
                    else
                        success = cover.scaledToWidth(STANDARD_EMBED_COVER_WIDTH, Qt::SmoothTransformation).save(cover_filepath);
                }

                if (success) {
                    qDebug() << "Successfully created temporary cover file" << cover_filepath << "(" << QFileInfo(cover_filepath).size() / 1024 << "KiB)";
                } else {
                    p_error_string = i18n("Could not create temporary cover file: %1").arg(cover_filepath);
                    qDebug() << "WARNING! Could not create temporary cover file" << cover_filepath;
                }
            } else {
                success = true;
            }
        }

        if (success)
            placeholders.insert(VAR_COVER_FILE, cover_filepath);
    }

    return parseScheme(scheme, placeholders);
}

const QString SchemeParser::parseFilenameScheme(const QString &text,
                                                const int cdno,
                                                const int nooftracks,
                                                const QString &artist,
                                                const QString &title,
                                                const QString &date,
                                                const QString &genre,
                                                const QString &suffix,
                                                const bool fat32_compatible,
                                                const bool replace_spaces_with_underscores)
{
    Placeholders placeholders;

    QString fartist = artist;
    QString ftitle = title;
    QString fdate = date;
    QString fgenre = genre;
    QString fsuffix = suffix;

    if (fat32_compatible) {
        fartist = makeFAT32FilenameCompatible(fartist);
        ftitle = makeFAT32FilenameCompatible(ftitle);
        fdate = makeFAT32FilenameCompatible(fdate);
        fgenre = makeFAT32FilenameCompatible(fgenre);
        fsuffix = makeFAT32FilenameCompatible(fsuffix);
    }

    if (replace_spaces_with_underscores) {
        fartist = replaceSpacesWithUnderscores(fartist);
        ftitle = replaceSpacesWithUnderscores(ftitle);
        fdate = replaceSpacesWithUnderscores(fdate);
        fgenre = replaceSpacesWithUnderscores(fgenre);
        fsuffix = replaceSpacesWithUnderscores(fsuffix);
    }

    placeholders.insert(VAR_ALBUM_ARTIST, fartist);
    placeholders.insert(VAR_ALBUM_TITLE, ftitle);
    placeholders.insert(VAR_DATE, fdate);
    placeholders.insert(VAR_GENRE, fgenre);
    placeholders.insert(VAR_SUFFIX, fsuffix);

    placeholders.insert(VAR_CD_NO, cdno);
    placeholders.insert(VAR_NO_OF_TRACKS, nooftracks);

    return parseScheme(text, placeholders);
}

void SchemeParser::parseInfoTextScheme(QStringList &text,
                                       const QString &artist,
                                       const QString &title,
                                       const QString &date,
                                       const QString &genre,
                                       const QString &mcn,
                                       const quint32 discid,
                                       const qreal size,
                                       const int length,
                                       const int nooftracks)
{
    Placeholders placeholders;

    placeholders.insert(VAR_ALBUM_ARTIST, artist);
    placeholders.insert(VAR_ALBUM_TITLE, title);
    placeholders.insert(VAR_DATE, date);
    placeholders.insert(VAR_GENRE, genre);
    placeholders.insert(VAR_MCN, mcn);
    placeholders.insert(VAR_AUDEX, AUDEX_VERSION_STRING);

    placeholders.insert(VAR_DISCID, QString("%1").arg(discid, 0, 16));
    placeholders.insert(VAR_CD_LENGTH, QString("%1:%2").arg(length / 60, 2, 10, QChar('0')).arg(length % 60, 2, 10, QChar('0')));
    placeholders.insert(VAR_CD_SIZE, size);
    placeholders.insert(VAR_NO_OF_TRACKS, nooftracks);
    placeholders.insert(VAR_TODAY, QDateTime::currentDateTime());
    placeholders.insert(VAR_NOW, QDateTime::currentDateTime());
    placeholders.insert(VAR_LINEBREAK, QString("\n"));

    text = parseScheme(text.join('\n'), placeholders).split('\n');
}

}
