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
