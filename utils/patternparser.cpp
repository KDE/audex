/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "patternparser.h"

#include <QDebug>
#include <QStandardPaths>

SaxHandler::SaxHandler()
    : QXmlDefaultHandler()
{
    trackno = 1;
    cdno = 0;
    trackoffset = 0;
    fat32compatible = false;
    discid = 0;
    size = 0;
    length = 0;
    nooftracks = 0;
    is_filename_pattern = false;
    is_command_pattern = false;
    is_simple_pattern = false;
    is_text_pattern = false;
    cover = nullptr;
    /*TEMP*/ found_suffix = false;
}

SaxHandler::~SaxHandler()
{
}

bool SaxHandler::startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);

    if (qName == VAR_FILENAME_PATTERN) {
        is_filename_pattern = true;
        return true;
    }
    if (qName == VAR_COMMAND_PATTERN) {
        is_command_pattern = true;
        return true;
    }
    if (qName == VAR_SIMPLE_PATTERN) {
        is_simple_pattern = true;
        return true;
    }
    if (qName == VAR_TEXT_PATTERN) {
        is_text_pattern = true;
        return true;
    }

    p_element.clear();

    if (qName == VAR_ALBUM_ARTIST) {
        if ((is_filename_pattern) || (is_simple_pattern)) {
            QString s = artist;
            if ((fat32compatible) || (IS_TRUE(atts.value("fat32compatible"))))
                s = make_fat32_compatible(s);
            else
                s = make_compatible(s);
            if ((replacespaceswithunderscores) || (IS_TRUE(atts.value("underscores"))))
                s = replace_spaces_with_underscores(s);
            // int QString::toInt(bool *ok, int base) const
            // If a conversion error occurs, *\a{ok} is set to \c false; otherwise
            // *\a{ok} is set to \c true.
            // http://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/tools/qstring.cpp#n6418
            bool ok;
            int left = atts.value("left").toInt(&ok);
            if ((ok) && (left > 0))
                s = s.left(left);
            if (IS_TRUE(atts.value("replace_char_list")))
                s = replace_char_list(atts, s);
            if (IS_TRUE(atts.value("lowercase")))
                s = s.toLower();
            else if (IS_TRUE(atts.value("uppercase")))
                s = s.toUpper();
            p_element += s;
        } else if (is_command_pattern) {
            p_element += make_compatible_2(artist);
        } else {
            p_element += artist;
        }
    }
    if (qName == VAR_ALBUM_TITLE) {
        if ((is_filename_pattern) || (is_simple_pattern)) {
            QString s = title;
            if ((fat32compatible) || (IS_TRUE(atts.value("fat32compatible"))))
                s = make_fat32_compatible(s);
            else
                s = make_compatible(s);
            if ((replacespaceswithunderscores) || (IS_TRUE(atts.value("underscores"))))
                s = replace_spaces_with_underscores(s);
            bool ok;
            int left = atts.value("left").toInt(&ok);
            if ((ok) && (left > 0))
                s = s.left(left);
            if (IS_TRUE(atts.value("replace_char_list")))
                s = replace_char_list(atts, s);
            if (IS_TRUE(atts.value("lowercase")))
                s = s.toLower();
            else if (IS_TRUE(atts.value("uppercase")))
                s = s.toUpper();
            p_element += s;
        } else if (is_command_pattern) {
            p_element += make_compatible_2(title);
        } else {
            p_element += title;
        }
    }
    if (qName == VAR_DATE) {
        if ((is_filename_pattern) || (is_simple_pattern)) {
            QString s = date;
            if ((fat32compatible) || (IS_TRUE(atts.value("fat32compatible"))))
                s = make_fat32_compatible(s);
            else
                s = make_compatible(s);
            if ((replacespaceswithunderscores) || (IS_TRUE(atts.value("underscores"))))
                s = replace_spaces_with_underscores(s);
            bool ok;
            int left = atts.value("left").toInt(&ok);
            if ((ok) && (left > 0))
                s = s.left(left);
            if (IS_TRUE(atts.value("replace_char_list")))
                s = replace_char_list(atts, s);
            if (IS_TRUE(atts.value("lowercase")))
                s = s.toLower();
            else if (IS_TRUE(atts.value("uppercase")))
                s = s.toUpper();
            p_element += s;
        } else if (is_command_pattern) {
            p_element += make_compatible_2(date);
        } else {
            p_element += date;
        }
    }
    if (qName == VAR_GENRE) {
        if ((is_filename_pattern) || (is_simple_pattern)) {
            QString s = genre;
            if ((fat32compatible) || (IS_TRUE(atts.value("fat32compatible"))))
                s = make_fat32_compatible(s);
            else
                s = make_compatible(s);
            if ((replacespaceswithunderscores) || (IS_TRUE(atts.value("underscores"))))
                s = replace_spaces_with_underscores(s);
            bool ok;
            int left = atts.value("left").toInt(&ok);
            if ((ok) && (left > 0))
                s = s.left(left);
            if (IS_TRUE(atts.value("replace_char_list")))
                s = replace_char_list(atts, s);
            if (IS_TRUE(atts.value("lowercase")))
                s = s.toLower();
            else if (IS_TRUE(atts.value("uppercase")))
                s = s.toUpper();
            p_element += s;
        } else if (is_command_pattern) {
            p_element += make_compatible_2(genre);
        } else {
            p_element += genre;
        }
    }
    if (qName == VAR_ENCODER) {
        p_element += encoder;
    }

    if ((is_filename_pattern) || (is_command_pattern) || (is_simple_pattern)) {
        if (qName == VAR_CD_NO) {
            if (cdno > 0) {
                bool ok;
                int l = atts.value("length").toInt(&ok);
                QChar fc = '0';
                if (!atts.value("fillchar").isEmpty())
                    fc = atts.value("fillchar").at(0);
                if (ok)
                    p_element += QString("%1").arg(cdno, l, 10, fc);
                else
                    p_element += QString("%1").arg(cdno);
            }
        }
    }
    if ((is_filename_pattern) || (is_command_pattern)) {
        if (qName == VAR_TRACK_ARTIST) {
            if (is_filename_pattern) {
                QString s = tartist;
                if ((fat32compatible) || (IS_TRUE(atts.value("fat32compatible"))))
                    s = make_fat32_compatible(s);
                else
                    s = make_compatible(s);
                if ((replacespaceswithunderscores) || (IS_TRUE(atts.value("underscores"))))
                    s = replace_spaces_with_underscores(s);
                bool ok;
                int left = atts.value("left").toInt(&ok);
                if ((ok) && (left > 0))
                    s = s.left(left);
                if (IS_TRUE(atts.value("lowercase")))
                    s = s.toLower();
                else if (IS_TRUE(atts.value("uppercase")))
                    s = s.toUpper();
                if (IS_TRUE(atts.value("replace_char_list")))
                    s = replace_char_list(atts, s);
                p_element += s;
            } else if (is_command_pattern) {
                p_element += make_compatible_2(tartist);
            } else {
                p_element += tartist;
            }
        }
        if (qName == VAR_TRACK_TITLE) {
            if (is_filename_pattern) {
                QString s = ttitle;
                if ((fat32compatible) || (IS_TRUE(atts.value("fat32compatible"))))
                    s = make_fat32_compatible(s);
                else
                    s = make_compatible(s);
                if ((replacespaceswithunderscores) || (IS_TRUE(atts.value("underscores"))))
                    s = replace_spaces_with_underscores(s);
                bool ok;
                int left = atts.value("left").toInt(&ok);
                if ((ok) && (left > 0))
                    s = s.left(left);
                if (IS_TRUE(atts.value("replace_char_list")))
                    s = replace_char_list(atts, s);
                if (IS_TRUE(atts.value("lowercase")))
                    s = s.toLower();
                else if (IS_TRUE(atts.value("uppercase")))
                    s = s.toUpper();
                p_element += s;
            } else if (is_command_pattern) {
                p_element += make_compatible_2(ttitle);
            } else {
                p_element += ttitle;
            }
        }
        if (qName == VAR_TRACK_NO) {
            int t;
            if (trackoffset > 1)
                t = trackno + trackoffset;
            else
                t = trackno;
            bool ok;
            int l = atts.value("length").toInt(&ok);
            QChar fc = '0';
            if (!atts.value("fillchar").isEmpty())
                fc = atts.value("fillchar").at(0);
            if (ok) {
                p_element += QString("%1").arg(t, l, 10, fc);
            } else {
                if (_2digitstracknum) {
                    p_element += QString("%1").arg(t, 2, 10, QChar('0'));
                } else {
                    p_element += QString("%1").arg(t);
                }
            }
        }
    }

    if ((is_filename_pattern) || (is_simple_pattern)) {
        if (qName == VAR_SUFFIX) { /*TEMP*/
            found_suffix = true;
            p_element += suffix;
        }
    }

    if (is_command_pattern) {
        if (qName == VAR_INPUT_FILE)
            p_element += "\"" + input + "\"";
        if (qName == VAR_OUTPUT_FILE)
            p_element += "\"" + output + "\"";
        if (qName == VAR_COVER_FILE) {
            QString format = STANDARD_EMBED_COVER_FORMAT;
            if (!atts.value("format").isEmpty())
                format = atts.value("format");

            // cover set by setCover
            if ((cover) && (!cover->supportedFormats().contains(format.toLatin1().toLower())))
                format = STANDARD_EMBED_COVER_FORMAT;

            QString filename;
            bool stop = false;
            if (demomode) {
                filename = tmppath + "audexcover.123." + format.toLower();

            } else {
                int x = -1;
                int y = -1;
                bool ok;
                if (!atts.value("x").isEmpty()) {
                    // when *ok is false, QString::toInt() often return 0
                    x = atts.value("x").toInt(&ok);
                    if (!ok)
                        x = -1;
                }
                if (!atts.value("y").isEmpty()) {
                    y = atts.value("y").toInt(&ok);
                    if (!ok)
                        y = -1;
                }

                QByteArray ba = QCryptographicHash::hash(QString(artist + title + date + QString("%1").arg(x * y) + format).toUtf8(), QCryptographicHash::Md5);
                QString md5 = ba.toHex();

                if (!stop)
                    filename = tmppath + "cover." + QString("%1").arg(md5) + '.' + format.toLower();

                QFileInfo finfo(filename);
                if ((!finfo.exists()) && (!stop)) {
                    bool success;
                    if ((!cover) || ((cover) && (cover->isEmpty()))) {
                        if (IS_TRUE(atts.value("usenocover"))) {
                            QImage c = QImage(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString("audex/images/nocover.png")));
                            if ((x != -1) && (y != -1)) {
                                c = c.scaled(x, y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                            }
                            success = c.save(filename, format.toLatin1());
                        } else {
                            stop = true;
                        }
                    } else {
                        success = cover->save(filename, QSize(x, y));
                    }

                    if (!stop) {
                        if (!success) {
                            qDebug() << "WARNING! Could not create temporary cover file" << filename;
                        } else {
                            qDebug() << "Successfully created temporary cover file" << filename << "(" << QFile(filename).size() / 1024 << "KiB)";
                        }
                    }
                }
            }

            if (!stop) {
                p_element = "\"" + filename + "\"";
            }
        }
    }

    if (is_text_pattern) {
        if (qName == VAR_CD_SIZE) {
            QChar iec;
            if (!atts.value("iec").isEmpty())
                iec = atts.value("iec").at(0);
            if ((iec != 'b') && (iec != 'k') && (iec != 'm') && (iec != 'g'))
                iec = 'm';
            bool ok;
            int p = atts.value("precision").toInt(&ok);
            if (!ok)
                p = 2;
            if (iec == 'b')
                p_element += QString("%1").arg(size, 0, 'f', p);
            else if (iec == 'k')
                p_element += QString("%1").arg(size / 1024.0f, 0, 'f', p);
            else if (iec == 'm')
                p_element += QString("%1").arg(size / (1024.0f * 1024.0f), 0, 'f', p);
            else if (iec == 'g')
                p_element += QString("%1").arg(size / (1024.0f * 1024.0f * 1024.0f), 0, 'f', p);
        }
        if (qName == VAR_CD_LENGTH)
            p_element += QString("%1:%2").arg(length / 60, 2, 10, QChar('0')).arg(length % 60, 2, 10, QChar('0'));
        if (qName == VAR_TODAY) {
            QString format;
            if (!atts.value("format").isEmpty())
                format = atts.value("format");
            if (format.isEmpty()) {
                p_element += QString("%1").arg(QDate::currentDate().toString());
            } else {
                p_element += QString("%1").arg(QDate::currentDate().toString(format));
            }
        }
        if (qName == VAR_NOW) {
            QString format;
            if (!atts.value("format").isEmpty())
                format = atts.value("format");
            if (format.isEmpty()) {
                p_element += QString("%1").arg(QDateTime::currentDateTime().toString());
            } else {
                p_element += QString("%1").arg(QDateTime::currentDateTime().toString(format));
            }
        }
        if (qName == VAR_LINEBREAK)
            p_element += '\n';
        if (qName == VAR_DISCID) {
            bool ok;
            int base = atts.value("base").toInt(&ok);
            if (!ok)
                base = 16;
            p_element += QString("%1").arg(discid, 0, base);
        }
    }

    if (qName == VAR_NO_OF_TRACKS)
        p_element += QString("%1").arg(nooftracks);
    if (qName == VAR_AUDEX)
        p_element += QString("Audex Version %1").arg(AUDEX_VERSION);

    if ((!p_element.isEmpty()) && (is_command_pattern)) {
        QString pre = atts.value("pre");
        QString post = atts.value("post");
        p_text += pre + p_element + post;

    } else {
        p_text += p_element;
    }

    return true;
}

bool SaxHandler::endElement(const QString &namespaceURI, const QString &localName, const QString &qName)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);

    if (qName == VAR_FILENAME_PATTERN) {
        is_filename_pattern = false;
        p_text.replace("//", "/");
        p_text = p_text.simplified();
        return true;
    }
    if (qName == VAR_COMMAND_PATTERN) {
        is_command_pattern = false;
        p_text.replace("//", "/");
        p_text = p_text.simplified();
        return true;
    }
    if (qName == VAR_SIMPLE_PATTERN) {
        is_simple_pattern = false;
        p_text.replace("//", "/");
        p_text = p_text.simplified();
        return true;
    }
    if (qName == VAR_TEXT_PATTERN) {
        is_text_pattern = false;
        return true;
    }

    return true;
}

bool SaxHandler::characters(const QString &ch)
{
    p_text += ch;
    return true;
}

bool SaxHandler::fatalError(const QXmlParseException &exception)
{
    qDebug() << QString("XML pattern parse error: Column %1 (%2)").arg(exception.columnNumber()).arg(exception.message());
    return false;
}

const QString SaxHandler::make_compatible(const QString &string)
{
    QString s = string;
    for (int i = 0; i < s.size(); i++) {
        switch (s[i].toLatin1()) {
        case '/':
        case '\\':
            s[i] = '_';
            break;

        case '"':
            s[i] = '\'';
            break;

        default:
            break;
        }
    }
    return s;
}

const QString SaxHandler::make_compatible_2(const QString &string)
{
    QString s = string;
    s.replace('"', "\\\"");
    return s;
}

// remove \ / : * ? " < > |
const QString SaxHandler::make_fat32_compatible(const QString &string)
{
    QString s = string;
    for (int i = 0; i < s.size(); i++) {
        switch (s[i].toLatin1()) {
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

const QString SaxHandler::replace_spaces_with_underscores(const QString &string)
{
    QString s = string;
    s.replace(' ', '_');
    return s;
}

const QString SaxHandler::replace_char_list(const QXmlAttributes &atts, const QString &string)
{
    int i;
    QString from, to, result;

    qDebug() << "starting replacement for: " << string;

    from = atts.value("replace_char_list_from");
    to = atts.value("replace_char_list_to");

    if (from.count() != to.count()) {
        qDebug() << "Could not replace if list length are not equal";
        return string;
    }

    result = string;
    for (i = 0; i < from.count(); i++) {
        result.replace(from.at(i), to.at(i));
    }

    qDebug() << "finished: " << result;

    return result;
}

PatternParser::PatternParser(QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(parent);
}

PatternParser::~PatternParser()
{
}

const QString PatternParser::parseFilenamePattern(const QString &pattern,
                                                  int trackno,
                                                  int cdno,
                                                  int trackoffset,
                                                  int nooftracks,
                                                  const QString &artist,
                                                  const QString &title,
                                                  const QString &tartist,
                                                  const QString &ttitle,
                                                  const QString &date,
                                                  const QString &genre,
                                                  const QString &suffix,
                                                  bool fat32compatible,
                                                  bool replacespaceswithunderscores,
                                                  bool _2digitstracknum)
{
    SaxHandler handler;
    handler.setTrackNo(trackno);
    handler.setCDNo(cdno);
    handler.setTrackOffset(trackoffset);
    handler.setNoOfTracks(nooftracks);
    handler.setArtist(artist);
    handler.setTitle(title);
    handler.setTrackArtist(tartist);
    handler.setTrackTitle(ttitle);
    handler.setDate(date);
    handler.setGenre(genre);
    handler.setSuffix(suffix);
    handler.setFAT32Compatible(fat32compatible);
    handler.setReplaceSpacesWithUnderscores(replacespaceswithunderscores);
    handler.set2DigitsTrackNum(_2digitstracknum);

    QXmlInputSource inputSource;
    inputSource.setData("<filenamepattern>" + p_xmlize_pattern(pattern) + "</filenamepattern>");
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    reader.parse(inputSource);

    return handler.text();
}

const QString PatternParser::parseCommandPattern(const QString &pattern,
                                                 const QString &input,
                                                 const QString &output,
                                                 int trackno,
                                                 int cdno,
                                                 int trackoffset,
                                                 int nooftracks,
                                                 const QString &artist,
                                                 const QString &title,
                                                 const QString &tartist,
                                                 const QString &ttitle,
                                                 const QString &date,
                                                 const QString &genre,
                                                 const QString &suffix,
                                                 CachedImage *cover,
                                                 bool fatcompatible,
                                                 const QString &tmppath,
                                                 const QString &encoder,
                                                 const bool demomode)
{
    SaxHandler handler;
    handler.setInputFile(input);
    handler.setOutputFile(output);
    handler.setTrackNo(trackno);
    handler.setCDNo(cdno);
    handler.setTrackOffset(trackoffset);
    handler.setNoOfTracks(nooftracks);
    handler.setArtist(artist);
    handler.setTitle(title);
    handler.setTrackArtist(tartist);
    handler.setTrackTitle(ttitle);
    handler.setDate(date);
    handler.setGenre(genre);
    handler.setSuffix(suffix);
    // cover is initialized!
    handler.setCover(cover);
    handler.setFAT32Compatible(fatcompatible);
    handler.setTMPPath(tmppath);
    handler.setDemoMode(demomode);
    handler.set2DigitsTrackNum(false);
    handler.setEncoder(encoder);

    QXmlInputSource inputSource;
    inputSource.setData("<commandpattern>" + p_xmlize_pattern(pattern) + "</commandpattern>");
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    reader.parse(inputSource);

    return handler.text();
}

const QString PatternParser::parseSimplePattern(const QString &pattern,
                                                int cdno,
                                                const int nooftracks,
                                                const QString &artist,
                                                const QString &title,
                                                const QString &date,
                                                const QString &genre,
                                                const QString &suffix,
                                                bool fat32compatible)
{
    SaxHandler handler;
    handler.setCDNo(cdno);
    handler.setNoOfTracks(nooftracks);
    handler.setArtist(artist);
    handler.setTitle(title);
    handler.setDate(date);
    handler.setGenre(genre);
    handler.setSuffix(suffix);
    handler.setFAT32Compatible(fat32compatible);
    handler.setReplaceSpacesWithUnderscores(false);
    handler.set2DigitsTrackNum(false);

    QXmlInputSource inputSource;
    inputSource.setData("<simplepattern>" + p_xmlize_pattern(pattern) + "</simplepattern>");
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    reader.parse(inputSource);

    return handler.text();
}

void PatternParser::parseInfoText(QStringList &text,
                                  const QString &artist,
                                  const QString &title,
                                  const QString &date,
                                  const QString &genre,
                                  const quint32 discid,
                                  const qreal size,
                                  const int length,
                                  const int nooftracks)
{
    SaxHandler handler;
    handler.setArtist(artist);
    handler.setTitle(title);
    handler.setDate(date);
    handler.setGenre(genre);
    handler.setDiscid(discid);
    handler.setSize(size);
    handler.setLength(length);
    handler.setNoOfTracks(nooftracks);
    handler.set2DigitsTrackNum(false);

    QXmlInputSource inputSource;
    inputSource.setData("<textpattern>" + p_xmlize_pattern(text.join("\n")) + "</textpattern>");
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    reader.parse(inputSource);

    text = handler.text().split('\n');
}

const QString PatternParser::p_xmlize_pattern(const QString &pattern)
{
    QString newpattern;

    QString name;
    int s = 0;
    for (int i = 0; i < pattern.length(); ++i) {
        if (pattern[i] == '&') {
            newpattern += "&amp;";
            continue;
        }

        switch (s) {
        // outside var
        case 0:
            if (pattern[i] == '$') {
                name.clear();
                s = 1;
                continue;
            }
            break;

        // inside var
        case 1:
            if (pattern[i] == '{') {
                s = 3;
            } else if (pattern[i] == '$') {
                newpattern += '$';
                s = 0;
            } else {
                s = 2;
                name += pattern[i];
            }
            continue;

        // inside simple var
        case 2:
            if (!pattern[i].isLetter()) {
                if (!name.trimmed().isEmpty())
                    newpattern += '<' + name + " />";
                name.clear();
                s = 0;
                if (pattern[i] == '$') {
                    name.clear();
                    s = 1;
                    continue;
                } else {
                    newpattern += pattern[i];
                }
                continue;
            }
            name += pattern[i];
            continue;

        // inside extended var
        case 3:
            if (pattern[i] == '}') {
                if (!name.trimmed().isEmpty())
                    newpattern += '<' + name + " />";
                name.clear();
                s = 0;
                continue;
            }
            name += pattern[i];
            continue;
        }

        newpattern += pattern[i];
    }

    // rest at the end?
    if ((s == 2) && (!name.trimmed().isEmpty()))
        newpattern += '<' + name + " />";

    return newpattern;
}
