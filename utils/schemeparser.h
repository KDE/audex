/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PLACEHOLDERPARSER_H
#define PLACEHOLDERPARSER_H

#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QLocale>
#include <QMetaType>
#include <QObject>
#include <QTemporaryFile>

#include <KLocalizedString>

#include "utils/parameters.h"

#define VAR_ALBUM_ARTIST "artist"
#define VAR_ALBUM_TITLE "title"
#define VAR_TRACK_ARTIST "tartist"
#define VAR_TRACK_TITLE "ttitle"
#define VAR_TRACK_NO "trackno"
#define VAR_CD_NO "cdno"
#define VAR_DATE "date"
#define VAR_GENRE "genre"
#define VAR_ISRC "isrc"
#define VAR_SUFFIX "suffix"
#define VAR_ENCODER "encoder"

#define VAR_INPUT_FILE "i"
#define VAR_OUTPUT_FILE "o"
#define VAR_COVER_FILE "cover"

#define VAR_MCN "mcn"
#define VAR_DISCID "discid"
#define VAR_CD_SIZE "size"
#define VAR_CD_LENGTH "length"
#define VAR_TODAY "today"
#define VAR_NOW "now"
#define VAR_LINEBREAK "br"

#define VAR_AUDEX "audex"
#define VAR_NO_OF_TRACKS "nooftracks"

#define STANDARD_EMBED_COVER_FORMAT "jpeg"
#define STANDARD_EMBED_COVER_WIDTH 600

typedef QMap<QString, QVariant> Placeholders;
typedef QMap<QString, Parameters> PlaceholdersParameters;

class SchemeParser : public QObject
{
    Q_OBJECT
public:
    explicit SchemeParser(QObject *parent = nullptr);
    ~SchemeParser() override;

    // placeholders_parameters: return the actually found placeholders with their parameters as QMap
    const QString parseScheme(const QString &scheme, const Placeholders &placeholders, PlaceholdersParameters *placeholders_parameters = nullptr);

    const QString parsePerTrackFilenameScheme(const QString &scheme,
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
                                              const bool twodigitstracknum);

    const QString parsePerTrackCommandScheme(const QString &scheme,
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
                                             const bool demomode = false);

    const QString parseFilenameScheme(const QString &scheme,
                                      const int cdno,
                                      const int nooftracks,
                                      const QString &artist,
                                      const QString &title,
                                      const QString &date,
                                      const QString &genre,
                                      const QString &suffix,
                                      const bool fat32compatible);

    void parseInfoTextScheme(QStringList &text,
                             const QString &artist,
                             const QString &title,
                             const QString &date,
                             const QString &genre,
                             const QString &mcn,
                             const quint32 discid,
                             const qreal size,
                             const int length,
                             const int nooftracks);

    inline bool error() const
    {
        return !p_error_string.isEmpty();
    }

    inline const QString errorString() const
    {
        return p_error_string;
    }

    // scheme: 1 == PerTrackFilename, 2 = PerTrackCommand, 3 == Filename, 4 == InfoText
    static const QString helpHTMLDoc(const int scheme)
    {
        QString result =
            "<html>"
            "<head>"
            "<style type=\"text/css\">"
            "* { font-size: 0.8em; }"
            "table { margin: 8px 0 8px; }"
            "table tr th { padding: 4px 6px; border-bottom: 1px solid; }"
            "table tr td { padding: 2px 2px; border-bottom: 1px dotted; }"
            "</style>"
            "</head>"
            "<body>";

        result.append(
            i18n("<p>The following placeholders will be replaced with their particular meaning:</p>"
                 "<table>"
                 "<tr><th>Placeholder</th><th>Description</th></tr>"
                 "<tr><td><tt>$artist</tt></td><td>The artist of the CD. If your CD is a compilation then this tag represents the title in most "
                 "cases.</td></tr>"
                 "<tr><td><tt>$title</tt></td><td>The title of the CD. If your CD is a compilation then this tag represents the subtitle in most "
                 "cases.</td></tr>"
                 "<tr><td><tt>$date</tt></td><td>The release date of the CD. In almost all cases this is the year.</td></tr>"
                 "<tr><td><tt>$genre</tt></td><td>The genre of the CD.</td></tr>"
                 "<tr><td><tt>$cdno</tt></td><td>The CD number of a multi-CD album. Often compilations consist of several CDs. <i>Note:</i> If the "
                 "multi-CD flag is <b>not</b> set for the current CD then this value will be empty.</td></tr>"
                 "<tr><td><tt>$nooftracks</tt></td><td>The total number of audio tracks of the CD.</td></tr>"
                 "<tr><td><tt>$encoder</tt></td><td>Encoder name and version.</td></tr>"
                 "<tr><td><tt>$audex</tt></td><td>Audex name and version.</td></tr>"
                 "</table>"));

        result.append(i18n(
            "<p>Placeholders in Audex can have parameters in the form key=value. E.g. <tt>${title lowercase=true}</tt>. In this example "
            "the title will be lowercased. The following general parameters can be used with placeholders:</p>"
            "<table>"
            "<tr><th>Key</th><th>Value</th><th>Description</th></tr>"
            "<tr><td><tt>lowercase</tt></td><td>true/false</td><td>The placeholder value will be lowercased.</td></tr>"
            "<tr><td><tt>uppercase</tt></td><td>true/false</td><td>The placeholder value will be uppercased.</td></tr>"
            "<tr><td><tt>underscores</tt></td><td>true/false</td><td>Replace the spaces of the placeholder value with underscores.</td></tr>"
            "<tr><td><tt>fat32compatible</tt></td><td>true/false</td><td>Replace illegal filename characters of a FAT32 filesystem with underscores.</td></tr>"
            "<tr><td><tt>replace_chars</tt></td><td>true/false</td><td>Replace characters. This parameter needs two additional keys of the same value size:"
            "<tt>replace_char_list_from</tt>, <tt>replace_char_list_to</tt>. These are lists of characters. The first character of"
            "<tt>replace_char_list_from</tt> will be replaced by the first character of <tt>replace_char_list_to</tt> and so on.</td></tr>"
            "<tr><td><tt>left</tt></td><td>Number</td><td>Take nth characters from the left of the placeholder value.</td></tr>"
            "<tr><td><tt>length</tt></td><td>Number</td><td>If the placeholder value is a number expand it to the given length. Furthermore you can define "
            "a fill character with the additional key <tt>fillchar</tt>. Default fillchar is '0'.</td></tr>"
            "<tr><td><tt>pre</tt></td><td>String</td><td>A string which will be placed <b>before</b> the value. Especially useful for command line parameters "
            "which should not be set at all if the value is empty.</td></tr>"
            "<tr><td><tt>post</tt></td><td>String</td><td>A string which will be placed <b>after</b> the value.</td></tr>"
            "</table>"));

        if (scheme == 1 || scheme == 2) {
            result.append(
                i18n("<table>"
                     "<tr><th>Placeholder</th><th>Description</th></tr>"
                     "<tr><td>$tartist</td><td>This is the artist of every track. It is especially useful on compilation CDs.</td></tr>"
                     "<tr><td>$ttitle</td><td>The track title. Normally each track on a CD has its own title, which is the name of the song.</td></tr>"
                     "<tr><td>$trackno</td><td>The track number. First track is 1.</td></tr>"
                     "<tr><td><tt>$isrc</tt></td><td>The International Standard Recording Code (ISRC) of the track (only available if supported by your "
                     "device).</td></tr>"
                     "</table>"));
        }

        if (scheme == 2) {
            result.append(
                i18n("<table>"
                     "<tr><th>Placeholder</th><th>Specific parameter</th><th>Description</th></tr>"
                     "<tr><td><tt>$i</tt></td><td></td><td>The temporary WAVE file (RIFF WAVE) created by Audex from CD audio track. This is the "
                     "input file for your command line encoder.</td></tr>"
                     "<tr><td><tt>$o</tt></td><td></td><td>The full output filename and path. Use it as the output for your command line encoder.</td></tr>"
                     "<tr><td><tt>$cover</tt></td><td><tt>format,x,y</tt></td><td>Filename of the cover file. If no cover is set, this will be empty. Mostly "
                     "useful in conjunction with the <tt>pre</tt> parameter. Image can be scaled with the <tt>x</tt> and <tt>y</tt> parameters. Possible image "
                     "<tt>formats</tt> are JPEG, PNG, GIF or BMP (Default: JPEG). <i><b>Note:</b> "
                     "LAME discards cover files larger than 128 KiB.</i></td></tr>"
                     "</table>"));
        }

        if (scheme == 4) {
            result.append(i18n(
                "<table>"
                "<tr><th>Placeholder</th><th>Specific parameter</th><th>Description</th></tr>"
                "<tr><td>$size</td><td><tt>iec,precision</tt></td><td>Prints the overall size of all extracted (encoded) music files (incl. the cover image "
                "file). With <tt>iec</tt> calculate (k)ibi, (m)ebi or (g)ibi. The additional parameter <tt>precision</tt> can define the number "
                "of decimal places.</td></tr>"
                "<tr><td>$length</td><td></td><td>Prints the relevant overall length of all extracted tracks. The format is min:sec.</td></tr>"
                "<tr><td>$nooftracks</td><td></td><td>Prints the total number of extracted tracks.</td></tr>"
                "<tr><td>$discid</td><td>base</td><td>Prints the CDDB discid in hexadecimal format.</td></tr>"
                "<tr><td><tt>$mcn</tt></td><td></td><td>The Media Catalog Number (MCN) of the CD (only available if supported by your device).</td></tr>"
                "<tr><td>$now</td><td><tt>format,locale</tt></td><td>Prints the current date and/or time. The parameter format specifies the output. "
                "Please consult official qt documentation (<tt>https://doc.qt.io/qt-5/qtime.html#toString</tt>, "
                "<tt>https://doc.qt.io/qt-6/qdate.html#toString</tt>) for the supported specifiers within the format string. With the additional key "
                "<tt>locale</tt> you can specify a language setting. By default your "
                "system locales are used. The locale has the format <tt>language_TERRITORY</tt>, e.g. <tt>en_EN</tt>. <tt>language</tt> is a lowercase, "
                "two-letter, ISO 639 language code and <tt>TERRITORY</tt> an uppercase, two-letter, ISO 3166 territory code.</td></tr>"
                "<tr><td>$br</td><td></td><td>Prints a linebreak.</td></tr>"
                "</table>"));
        }

        result.append(
            "</body>"
            "</html>");

        return result;
    }

Q_SIGNALS:
    void error(const QString &message, const QString &details = QString());

private:
    const Parameters parse_keyvalue_stringlist(const QStringList &keyvaluepairs);

    QString p_error_string;

    inline const QString
    customize_placeholder_value(const QString &string, bool fat32compatible = false, bool compatible2 = true, bool replacespaceswithunderscores = false)
    {
        QString tmp = string;
        if (fat32compatible)
            tmp = make_fat32_compatible(tmp);
        else if (compatible2)
            tmp = make_compatible_2(tmp);
        else
            tmp = make_compatible(tmp);
        if (replacespaceswithunderscores)
            tmp = replace_spaces_with_underscores(tmp);
        return tmp;
    }

    const QString make_compatible(const QString &string);
    const QString make_compatible_2(const QString &string);
    const QString make_fat32_compatible(const QString &string);
    const QString replace_spaces_with_underscores(const QString &string);
    const QString replace_char_list(const QString &from, const QString &to, const QString &string);
};

#endif
