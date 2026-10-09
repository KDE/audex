/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// Command schemes: the "$artist" style strings of the Audex profiles, used
// for the external encoder and for the command hook that runs after a rip.
// Commands are started without a shell, so a scheme is translated into a
// list of arguments once, before the rip: the WAVE data arrives on standard
// input ("-") and %o stands for the output file.
//
// This happens in two steps, because the album is known before the rip and
// the track only when its file is opened:
//   parseCommandScheme()  album values, $i and $o        (before the rip)
//   substituteValues()    $ttitle and the other track    (per output file)
//                         placeholders
//
// The placeholder names are the ones of utils/schemeparser.h.

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

namespace Audex::Encoding
{

struct CommandIssue {
    enum class Kind {
        UnknownName, // a typo or a value Audex does not have
        HasParameters, // parameters are a feature of the filename schemes
        CoverFile // Audex embeds the cover into the files, there is no cover file
    };
    Kind kind;
    QString written; // the placeholder as written in the scheme
};

struct CommandScheme {
    QStringList arguments; // ready for QProcess, track placeholders still in
    QList<CommandIssue> issues; // what cannot be filled in, one entry per problem
};

// Placeholders the engine fills in per output file (see setTrackValues() in
// encoding/encoder.h).
QStringList trackCommandVariables();
QMap<QString, QString> trackValues(const QString &artist, const QString &title, int trackNumber, const QString &isrc);

// $name and ${name}. The name is read as a whole, so $isrc is not $i followed
// by "src"; unknown names and placeholders with parameters stay as they are
// and are reported. Values are put into the arguments after the scheme was
// split, so a quote or a space in a title cannot change the command line; %
// and $ in a value are escaped for the steps that follow.
CommandScheme parseCommandScheme(const QString &scheme, const QMap<QString, QString> &values);

// Looks for %o with %% as an escaped percent sign. Values are escaped when
// they are filled in, so a value can never look like the placeholder.
bool hasOutputFilePlaceholder(const QStringList &arguments);

// The last step: fills in `values`, turns $$ into a dollar sign and leaves
// everything else alone.
QStringList substituteValues(const QStringList &arguments, const QMap<QString, QString> &values);

// The arguments as one line, for the log. Quoting is the one
// QProcess::splitCommand() understands.
QString commandToString(const QStringList &arguments);

// The command hook of a profile, run after a rip: the scheme is split with
// QProcess rules, then the album values, $dir (output directory) and $$ (a
// dollar sign) are filled in per argument, so a space or a quote in a value
// can never change the command line. An argument that is exactly $files
// expands to the written files, one argument per file.
QStringList hookCommandArguments(const QString &command,
                                 const QMap<QString, QString> &albumVars,
                                 const QStringList &files,
                                 const QString &outputDir);

// What a hook command uses that cannot be filled in. Besides $files and $dir
// the hook knows the album values only: there is no $i/$o and no track, and
// parameters are a feature of the filename schemes.
QList<CommandIssue> checkHookCommand(const QString &command, const QMap<QString, QString> &values);

// A preset command for the custom encoder (encoderpresets.json)
struct EncoderPreset {
    QString name;
    QString command;
    QString suffix;
};

// encoderpresets.json: the user's own (~/.local/share/audex) first, then the
// installed one (share/audex) - the first file with a given preset name wins.
// Empty if no file is found; that is logged once.
QList<EncoderPreset> encoderPresets();

}
