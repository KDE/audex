/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QLocale>
#include <QMetaType>
#include <QObject>
#include <QStandardPaths>
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

#define VAR_MCN "mcn"
#define VAR_DISCID "discid"
#define VAR_CD_SIZE "size"
#define VAR_CD_LENGTH "length"
#define VAR_TODAY "today"
#define VAR_NOW "now"
#define VAR_LINEBREAK "br"

#define VAR_AUDEX "audex"
#define VAR_NO_OF_TRACKS "nooftracks"

typedef QMap<QString, QVariant> Placeholders;
typedef QMap<QString, Parameters> PlaceholdersParameters;

class SchemeParser
{
public:
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
                                              const bool fat32_compatible = false,
                                              const bool replace_spaces_with_underscores = false,
                                              const bool two_digits_tracknum = true);

    const QString parseFilenameScheme(const QString &scheme,
                                      const int cdno,
                                      const int nooftracks,
                                      const QString &artist,
                                      const QString &title,
                                      const QString &date,
                                      const QString &genre,
                                      const QString &suffix,
                                      const bool fat32_compatible = false,
                                      const bool replace_spaces_with_underscores = false);

    bool error() const
    {
        return !p_error_string.isEmpty();
    }

    const QString errorString() const
    {
        return p_error_string;
    }

    // names of the placeholders the last parse could not fill in (unknown names
    // are left out of the result, but not treated as an error)
    const QStringList unknownPlaceholders() const
    {
        return p_unknown_placeholders;
    }

    // scheme: 1 == PerTrackFilename, 2 == PerTrackCommand, 3 == Filename, 4 == HookCommand
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

        // parameters are a feature of the filename schemes; commands have none
        if (scheme == 1 || scheme == 3)
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
                "<tr><td><tt>pre</tt></td><td>String</td><td>A string which will be placed <b>before</b> the value.</td></tr>"
                "<tr><td><tt>post</tt></td><td>String</td><td>A string which will be placed <b>after</b> the value.</td></tr>"
                "<tr><td><tt>omit_if_empty</tt></td><td>true/false</td><td>The placeholder is left out completely, including <tt>pre</tt> and <tt>post</tt>, if its value "
                "is empty. E.g. <tt>${cdno omit_if_empty=true pre=\"CD \" post=\"/\"}</tt> adds a CD folder only when a CD number is set.</td></tr>"
                "</table>"));

        if (scheme == 1) {
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
                     "<tr><th>Placeholder</th><th>Description</th></tr>"
                     "<tr><td><tt>$i</tt></td><td>The audio data. Audex does not write a temporary file any more: it sends the audio to the command as "
                     "WAVE (RIFF WAVE) on standard input, and <tt>$i</tt> becomes the single <tt>-</tt> that tells most encoders to read from there. If "
                     "your encoder wants the input somewhere else (for example <tt>-i -</tt> or <tt>pipe:0</tt>), write that instead of "
                     "<tt>$i</tt>.</td></tr>"
                     "<tr><td><tt>$o</tt></td><td>The file the command has to write.</td></tr>"
                     "<tr><td><tt>$tartist</tt>, <tt>$ttitle</tt>, <tt>$trackno</tt>, <tt>$isrc</tt></td><td>Track data. They are filled in when the "
                     "file of that track is opened. A single file rip (disc image) has no track of its own and uses the album data.</td></tr>"
                     "</table>"
                     "<p>A command is not run in a shell: it is split into arguments once, and the values are put into those arguments afterwards. A quote "
                     "or a space in a track title can therefore not change the command line, and <tt>$$</tt> is a plain dollar sign. Parameters in "
                     "placeholders (<tt>${title lowercase=true}</tt>) are not supported here, and <tt>$cover</tt> cannot be filled in because Audex embeds the "
                     "cover itself instead of writing a file. A command that uses them is refused before the rip starts.</p>"));
        }

        if (scheme == 4) {
            result.append(
                i18n("<table>"
                     "<tr><th>Placeholder</th><th>Description</th></tr>"
                     "<tr><td><tt>%f</tt></td><td>The list of the written audio files. As an argument of its own it becomes one argument per file, "
                     "inside a longer argument the file names are joined with spaces.</td></tr>"
                     "<tr><td><tt>%d</tt></td><td>The output directory the files were written to.</td></tr>"
                     "<tr><td><tt>%%</tt></td><td>A plain percent sign.</td></tr>"
                     "</table>"
                     "<p>The command runs after a successfully finished rip. It is not run in a shell: it is split into arguments once, and the values "
                     "are put into those arguments afterwards, so no quoting is needed for them. Only the album values above are available here — there "
                     "is no single track and no <tt>$i</tt>/<tt>$o</tt> — parameters in placeholders (<tt>${title lowercase=true}</tt>) are not "
                     "supported, and a name Audex does not know is left in the command as it is.</p>"));
        }

        result.append(
            "</body>"
            "</html>");

        return result;
    }

    static const QString makeFAT32FilenameCompatible(const QString &string)
    {
        const QString invalidChars = R"(<>:"/\|?*)";
        QString result;

        for (QChar ch : string) {
            if (invalidChars.contains(ch) || ch.unicode() < 0x20) {
                result += '_';
            } else {
                result += ch;
            }
        }

        static const QStringList reservedNames = {"CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
                                                  "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

        QString baseName = result.section('.', 0, 0).toUpper();
        if (reservedNames.contains(baseName)) {
            result = "_" + result;
        }

        if (result.length() > 255)
            result = result.left(255);

        return result;
    }

    static const QString replaceSpacesWithUnderscores(const QString &string)
    {
        QString result = string;
        result.replace(' ', '_');
        return result;
    }

    static const QString replaceCharList(const QString &from, const QString &to, const QString &string)
    {
        if (from.length() != to.length()) {
            qDebug() << "Could not replace if list length are not equal";
            return string;
        }
        QString result = string;
        for (int i = 0; i < from.length(); i++) {
            result = result.replace(from.at(i), to.at(i));
        }
        return result;
    }

private:
    QString p_error_string;
    QStringList p_unknown_placeholders;

    void noteUnknownPlaceholder(const QString &name)
    {
        if (!name.isEmpty() && !p_unknown_placeholders.contains(name))
            p_unknown_placeholders.append(name);
    }
};
