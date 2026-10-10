/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "utils/encodercommand.h"

using namespace Audex::Encoding;
using namespace Qt::StringLiterals;

namespace
{

QString describeArguments(const QStringList &arguments)
{
    return u"["_s + arguments.join(u"|"_s) + u']';
}

const QMap<QString, QString> Album{{u"artist"_s, u"AC/DC"_s}, {u"title"_s, u"100% $hits"_s}};
const QStringList Files{u"/music/a b.flac"_s, u"/music/c.flac"_s};

}

AUDEX_TEST("hook command: $files and $dir")
{
    const QStringList args = hookCommandArguments(u"tool --dir=$dir $files ${files}"_s, Album, Files, u"/music"_s);
    const QStringList expected{u"tool"_s, u"--dir=/music"_s, u"/music/a b.flac"_s, u"/music/c.flac"_s, u"/music/a b.flac"_s, u"/music/c.flac"_s};
    AUDEX_CHECK_MSG(t, args == expected, describeArguments(args));
}

AUDEX_TEST("hook command: $files inside an argument stays one argument")
{
    const QStringList args = hookCommandArguments(u"tool \"list: $files\""_s, Album, Files, u"/music"_s);
    const QStringList expected{u"tool"_s, u"list: /music/a b.flac /music/c.flac"_s};
    AUDEX_CHECK_MSG(t, args == expected, describeArguments(args));
}

AUDEX_TEST("hook command: values and percent signs are kept as they are")
{
    const QStringList args = hookCommandArguments(u"notify \"$artist - $title\" 50% %f %d $$dir"_s, Album, Files, u"/music"_s);
    const QStringList expected{u"notify"_s, u"AC/DC - 100% $hits"_s, u"50%"_s, u"%f"_s, u"%d"_s, u"$dir"_s};
    AUDEX_CHECK_MSG(t, args == expected, describeArguments(args));
}

AUDEX_TEST("hook command: check knows $files and $dir")
{
    AUDEX_CHECK(t, checkHookCommand(u"tool $files ${dir} $artist"_s, Album).isEmpty());
    const QList<CommandIssue> issues = checkHookCommand(u"tool $file ${dir x=1}"_s, Album);
    AUDEX_EQUAL(t, qint64(issues.size()), qint64(2));
}

AUDEX_TEST("encoder command: literal text, values and track placeholders")
{
    const CommandScheme scheme = parseCommandScheme(u"enc \"--title=$title\" -c 50% ${ttitle} $o"_s, {{u"title"_s, u"100% $x"_s}});
    AUDEX_CHECK(t, scheme.issues.isEmpty());
    const QStringList album{u"enc"_s, u"--title=100%% $$x"_s, u"-c"_s, u"50%%"_s, u"${ttitle}"_s, u"%o"_s};
    AUDEX_CHECK_MSG(t, scheme.arguments == album, describeArguments(scheme.arguments));

    const QStringList track = substituteValues(scheme.arguments, {{u"ttitle"_s, u"A $b 5%"_s}});
    const QStringList expected{u"enc"_s, u"--title=100%% $x"_s, u"-c"_s, u"50%%"_s, u"A $b 5%%"_s, u"%o"_s};
    AUDEX_CHECK_MSG(t, track == expected, describeArguments(track));
}

AUDEX_TEST("encoder command: parameters and syntax errors are reported")
{
    const CommandScheme parameters = parseCommandScheme(u"enc ${title lowercase=true} $o"_s, {{u"title"_s, u"x"_s}});
    AUDEX_EQUAL(t, qint64(parameters.issues.size()), qint64(1));
    AUDEX_CHECK(t, parameters.issues.value(0).kind == CommandIssue::Kind::HasParameters);
    AUDEX_EQUAL(t, qint64(parameters.arguments.size()), qint64(3));

    const CommandScheme syntax = parseCommandScheme(u"enc ${title $o"_s, {});
    AUDEX_CHECK(t, syntax.issues.value(0).kind == CommandIssue::Kind::Syntax);
    AUDEX_CHECK(t, checkHookCommand(u"tool ${dir"_s, {}).value(0).kind == CommandIssue::Kind::Syntax);
}
