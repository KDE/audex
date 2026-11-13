/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>
#include <QStringBuilder>
#include <QStringView>

// The pieces of the rip log that are written by Rip::formatReport() and
// Cdda::Toc::describe() and read back by Verify::parseRipLog(). Both sides
// take them from here, so the format is spelled out once.
namespace Audex::RipLogFormat
{

inline constexpr QStringView HeaderMark = u" extraction log from "; // "<application> extraction log from <date>"

// the table of contents: indented by four spaces
inline constexpr QStringView TocIndent = u"    ";
inline constexpr QStringView LeadOut = u"Lead-out: "; // "Lead-out: <lba> (<msf>)"
inline constexpr QStringView TrackNote = u"Track "; // "Track <number>: data, session 2, ..."
inline constexpr QStringView TocColumnSeparator = u"|";

// a track section: its heading at the start of the line, its lines indented by five spaces
inline constexpr QStringView TrackHeading = u"Track "; // "Track <number, two digits>"
inline constexpr QStringView HiddenTrackHeading = u"Hidden track one audio";
inline constexpr QStringView TrackIndent = u"     ";
inline constexpr QStringView Filename = u"Filename "; // "Filename <path>"
inline constexpr QStringView Range = u"Range LBA "; // "Range LBA <first> - <last> (<msf>)"
inline constexpr QStringView CopyCrc = u"Copy CRC "; // "Copy CRC <8 hex digits> ..."

inline QString tocLine(QStringView key, const QString &rest)
{
    return TocIndent % key % rest;
}

inline QString trackLine(QStringView key, const QString &rest)
{
    return TrackIndent % key % rest;
}

}
