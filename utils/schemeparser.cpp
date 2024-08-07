/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "schemeparser.h"

#include <QDebug>
#include <QStandardPaths>
#include "audex-version.h"

#if QT_VERSION >= 0x060000
#define IS_TRUE(val) (val.typeId() == QMetaType::Bool && val.toBool())
#define IS_INT(val)                                                                                                                                            \
    (val.typeId() == QMetaType::Int || val.typeId() == QMetaType::UInt || val.typeId() == QMetaType::LongLong || val.typeId() == QMetaType::ULongLong)
#define IS_DATETIME(val) (val.typeId() == QMetaType::QDateTime || val.typeId() == QMetaType::QDate || val.typeId() == QMetaType::QTime)
#endif

#if QT_VERSION < 0x060000
#define IS_TRUE(val) (val.type() == QVariant::Bool && val.toBool())
#define IS_INT(val) (val.type() == QVariant::Int || val.type() == QVariant::UInt || val.type() == QVariant::LongLong || val.type() == QVariant::ULongLong)
#define IS_DATETIME(val) (val.type() == QVariant::DateTime || val.type() == QVariant::Date || val.type() == QVariant::Time)
#endif

SchemeParser::SchemeParser(QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(parent);
}

SchemeParser::~SchemeParser()
{
}

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
                                value = replace_spaces_with_underscores(value.toString());

                            if (key == "fat32compatible" && IS_TRUE(parameters.value("fat32compatible")))
                                value = make_fat32_compatible(value.toString());

                            if ((key == "replace_char_list" && IS_TRUE(parameters.value("replace_char_list")))
                                || (key == "replace_chars" && IS_TRUE(parameters.value("replace_chars")))) {
                                if (parameters.contains("replace_char_list_from") && parameters.contains("replace_char_list_to")
                                    && parameters.value("replace_char_list_from").toString().length()
                                        == parameters.value("replace_char_list_to").toString().length())
                                    value = QVariant(replace_char_list(parameters.value("replace_char_list_from").toString(),
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
                                                        const bool fat32compatible,
                                                        const bool replacespaceswithunderscores,
                                                        const bool twodigitstracknum)
{
    Placeholders placeholders;

    placeholders.insert(VAR_ALBUM_ARTIST, customize_placeholder_value(artist, fat32compatible, replacespaceswithunderscores));
    placeholders.insert(VAR_ALBUM_TITLE, customize_placeholder_value(title, fat32compatible, replacespaceswithunderscores));
    placeholders.insert(VAR_TRACK_ARTIST, customize_placeholder_value(tartist, fat32compatible, replacespaceswithunderscores));
    placeholders.insert(VAR_TRACK_TITLE, customize_placeholder_value(ttitle, fat32compatible, replacespaceswithunderscores));
    placeholders.insert(VAR_DATE, customize_placeholder_value(date, fat32compatible, replacespaceswithunderscores));
    placeholders.insert(VAR_GENRE, customize_placeholder_value(genre, fat32compatible, replacespaceswithunderscores));
    placeholders.insert(VAR_ISRC, customize_placeholder_value(isrc, fat32compatible, replacespaceswithunderscores));

    placeholders.insert(VAR_SUFFIX, suffix);

    int tn = trackno;
    if (trackoffset > 1)
        tn += trackoffset;

    if (twodigitstracknum)
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
    placeholders.insert(VAR_OUTPUT_FILE, mask_inner_quotes(output));

    placeholders.insert(VAR_ALBUM_ARTIST, mask_inner_quotes(customize_placeholder_value(artist)));
    placeholders.insert(VAR_ALBUM_TITLE, mask_inner_quotes(customize_placeholder_value(title)));
    placeholders.insert(VAR_TRACK_ARTIST, mask_inner_quotes(customize_placeholder_value(tartist)));
    placeholders.insert(VAR_TRACK_TITLE, mask_inner_quotes(customize_placeholder_value(ttitle)));
    placeholders.insert(VAR_DATE, mask_inner_quotes(customize_placeholder_value(date)));
    placeholders.insert(VAR_GENRE, mask_inner_quotes(customize_placeholder_value(genre)));
    placeholders.insert(VAR_ISRC, customize_placeholder_value(isrc));
    placeholders.insert(VAR_ENCODER, mask_inner_quotes(customize_placeholder_value(encoder)));

    placeholders.insert(VAR_AUDEX, customize_placeholder_value(AUDEX_VERSION_STRING));

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
                                                const bool fat32compatible)
{
    Placeholders placeholders;

    placeholders.insert(VAR_ALBUM_ARTIST, customize_placeholder_value(artist, fat32compatible));
    placeholders.insert(VAR_ALBUM_TITLE, customize_placeholder_value(title, fat32compatible));
    placeholders.insert(VAR_DATE, customize_placeholder_value(date, fat32compatible));
    placeholders.insert(VAR_GENRE, customize_placeholder_value(genre, fat32compatible));

    placeholders.insert(VAR_SUFFIX, suffix);

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

const QString SchemeParser::make_compatible(const QString &string)
{
    QString s = string;
    for (int i = 0; i < s.size(); i++) {
        switch (s[i].unicode()) {
        //case '/':
        case '\\':
            s[i] = '_';
            break;
        // case '"':
        //     s[i] = '\'';
        //     break;
        default:
            break;
        }
    }
    return s;
}

// remove \ / : * ? " < > |
const QString SchemeParser::make_fat32_compatible(const QString &string)
{
    QString s = string;
    for (int i = 0; i < s.size(); i++) {
        switch (s[i].unicode()) {
        case '\\':
        case '/':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|':
            s[i] = '_';
            break;

        default:
            break;
        }
    }
    return s;
}

const QString SchemeParser::replace_spaces_with_underscores(const QString &string)
{
    QString s = string;
    s.replace(' ', '_');
    return s;
}

const QString SchemeParser::replace_char_list(const QString &from, const QString &to, const QString &string)
{
    qDebug() << "starting replacement for:" << string;

    if (from.count() != to.count()) {
        qDebug() << "Could not replace if list length are not equal";
        return string;
    }

    QString result = string;
    for (int i = 0; i < from.count(); i++) {
        result = result.replace(from.at(i), to.at(i));
    }

    qDebug() << "finished:" << result;

    return result;
}
