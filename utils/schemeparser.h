/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>
#include <QStringList>

#include "utils/encodercommand.h"
#include "utils/scheme.h"

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

#define VAR_HOOK_FILES "files"
#define VAR_HOOK_OUTPUT_DIR "dir"

#define VAR_MCN "mcn"
#define VAR_DISCID "discid"
#define VAR_CD_SIZE "size"
#define VAR_CD_LENGTH "length"
#define VAR_TODAY "today"
#define VAR_NOW "now"
#define VAR_LINEBREAK "br"

#define VAR_AUDEX "audex"
#define VAR_NO_OF_TRACKS "nooftracks"

typedef QMap<QString, QString> Placeholders;

// Filename schemes (see utils/scheme.h for the syntax) and the user visible
// texts of all schemes
class SchemeParser
{
public:
    // Fills in the placeholders and applies their parameters. Empty on error.
    const QString parseScheme(const QString &scheme, const Placeholders &placeholders);

    bool error() const
    {
        return !p_error_string.isEmpty();
    }

    const QString errorString() const
    {
        return p_error_string;
    }

    // what the last parse left out or ignored, one text per problem
    const QStringList warnings() const
    {
        return p_warnings;
    }

    static QString errorText(const Audex::Scheme::Error &error);
    static QString commandIssueText(const Audex::Encoding::CommandIssue &issue);

    // scheme: 1 == PerTrackFilename, 2 == PerTrackCommand, 3 == Filename, 4 == HookCommand
    static QString helpHTMLDoc(const int scheme);

private:
    QString p_error_string;
    QStringList p_warnings;
};
