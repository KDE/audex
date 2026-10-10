/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "schemeparser.h"

#include <KLocalizedString>

const QString SchemeParser::parseScheme(const QString &scheme, const Placeholders &placeholders)
{
    p_error_string.clear();
    p_warnings.clear();

    const Audex::Scheme::Rendered rendered = Audex::Scheme::renderFileName(scheme, placeholders);
    if (rendered.error) {
        p_error_string = errorText(rendered.error);
        return QString();
    }
    for (const QString &name : rendered.unknownNames)
        p_warnings << i18n("%1 is not available in this scheme and is left out.", name);
    for (const auto &[placeholder, key] : rendered.unknownParameters)
        p_warnings << i18n("%1: the parameter %2 is unknown and ignored.", placeholder, key);
    return rendered.text;
}

QString SchemeParser::errorText(const Audex::Scheme::Error &error)
{
    using Kind = Audex::Scheme::Error::Kind;
    const int position = int(error.position) + 1;
    switch (error.kind) {
    case Kind::UnclosedBrace:
        return i18n("Unclosed brace at position %1.", position);
    case Kind::IllegalCharacter:
        return i18n("Illegal character \"%1\" at position %2.", QString(error.character), position);
    case Kind::UnclosedQuote:
        return i18n("Unclosed quote at position %1.", position);
    case Kind::MissingValue:
        return i18n("Parameter without a value at position %1.", position);
    case Kind::ReplaceCharsLength:
        return i18n("Position %1: replace_char_list_from and replace_char_list_to must have the same number of characters.", position);
    case Kind::None:
        break;
    }
    return QString();
}

QString SchemeParser::commandIssueText(const Audex::Encoding::CommandIssue &issue)
{
    switch (issue.kind) {
    case Audex::Encoding::CommandIssue::Kind::HasParameters:
        return i18n("%1: parameters are not supported in a command.", issue.written);
    case Audex::Encoding::CommandIssue::Kind::CoverFile:
        return i18n("%1 cannot be filled in: Audex embeds the cover itself and does not write a cover file.", issue.written);
    case Audex::Encoding::CommandIssue::Kind::Syntax:
        return errorText(issue.syntax);
    case Audex::Encoding::CommandIssue::Kind::UnknownName:
        break;
    }
    return i18n("%1 is not a known placeholder.", issue.written);
}

QString SchemeParser::helpHTMLDoc(const int scheme)
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
             "<tr><td><tt>$discid</tt>, <tt>$mcn</tt></td><td>The CDDB disc ID and the Media Catalog Number of the CD.</td></tr>"
             "<tr><td><tt>$today</tt>, <tt>$now</tt></td><td>The current date (YYYY-MM-DD) and time (hh-mm-ss).</td></tr>"
             "</table>"));

    // parameters are a feature of the filename schemes; commands have none
    if (scheme == 1 || scheme == 3)
        result.append(i18n(
            "<p>Placeholders in Audex can have parameters in the form key=value. E.g. <tt>${title lowercase=true}</tt>. In this example "
            "the title will be lowercased. The parameters are applied in the order they are written, <tt>pre</tt> and <tt>post</tt> are "
            "added last. The following general parameters can be used with placeholders:</p>"
            "<table>"
            "<tr><th>Key</th><th>Value</th><th>Description</th></tr>"
            "<tr><td><tt>lowercase</tt></td><td>true/false</td><td>The placeholder value will be lowercased.</td></tr>"
            "<tr><td><tt>uppercase</tt></td><td>true/false</td><td>The placeholder value will be uppercased.</td></tr>"
            "<tr><td><tt>underscores</tt></td><td>true/false</td><td>Replace the spaces of the placeholder value with underscores.</td></tr>"
            "<tr><td><tt>fat32compatible</tt></td><td>true/false</td><td>Replace illegal filename characters of a FAT32 filesystem with underscores.</td></tr>"
            "<tr><td><tt>replace_chars</tt></td><td>true/false</td><td>Replace characters. This parameter needs two additional keys of the same value size: "
            "<tt>replace_char_list_from</tt>, <tt>replace_char_list_to</tt>. These are lists of characters. The first character of "
            "<tt>replace_char_list_from</tt> will be replaced by the first character of <tt>replace_char_list_to</tt> and so on.</td></tr>"
            "<tr><td><tt>left</tt></td><td>Number</td><td>Take nth characters from the left of the placeholder value.</td></tr>"
            "<tr><td><tt>length</tt></td><td>Number</td><td>If the placeholder value is a number expand it to the given length (e.g. <tt>${trackno "
            "length=3}</tt>). Furthermore you can define "
            "a fill character with the additional key <tt>fillchar</tt>. Default fillchar is '0'.</td></tr>"
            "<tr><td><tt>pre</tt></td><td>String</td><td>A string which will be placed <b>before</b> the value.</td></tr>"
            "<tr><td><tt>post</tt></td><td>String</td><td>A string which will be placed <b>after</b> the value.</td></tr>"
            "<tr><td><tt>omit_if_empty</tt></td><td>true/false</td><td>The placeholder is left out completely, including <tt>pre</tt> and <tt>post</tt>, if "
            "its value "
            "is empty. E.g. <tt>${cdno omit_if_empty=true pre=\"CD \" post=\"/\"}</tt> adds a CD folder only when a CD number is set.</td></tr>"
            "</table>"
            "<p>Write <tt>$$</tt> for a dollar sign. A quote inside a quoted value is written twice, e.g. <tt>pre='it''s '</tt>.</p>"));

    if (scheme == 1) {
        result.append(
            i18n("<table>"
                 "<tr><th>Placeholder</th><th>Description</th></tr>"
                 "<tr><td><tt>$tartist</tt></td><td>This is the artist of every track. It is especially useful on compilation CDs.</td></tr>"
                 "<tr><td><tt>$ttitle</tt></td><td>The track title. Normally each track on a CD has its own title, which is the name of the song.</td></tr>"
                 "<tr><td><tt>$trackno</tt></td><td>The track number. First track is 1.</td></tr>"
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
                 "<tr><td><tt>$files</tt></td><td>The list of the written audio files. As an argument of its own it becomes one argument per file, "
                 "inside a longer argument the file names are joined with spaces.</td></tr>"
                 "<tr><td><tt>$dir</tt></td><td>The output directory the files were written to.</td></tr>"
                 "<tr><td><tt>$$</tt></td><td>A plain dollar sign.</td></tr>"
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
