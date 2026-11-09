/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "encoding/registry.h"
#include "utils/encodercommand.h"

#include <QFile>
#include <QTemporaryDir>

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
    const QStringList album{u"enc"_s, u"--title=100% $$x"_s, u"-c"_s, u"50%"_s, u"${ttitle}"_s, u"${o}"_s};
    AUDEX_CHECK_MSG(t, scheme.arguments == album, describeArguments(scheme.arguments));

    const QStringList track = substituteValues(scheme.arguments, {{u"ttitle"_s, u"A $b 5%"_s}, {u"o"_s, u"/out/a $o %o.m4a"_s}});
    const QStringList expected{u"enc"_s, u"--title=100% $x"_s, u"-c"_s, u"50%"_s, u"A $b 5%"_s, u"/out/a $o %o.m4a"_s};
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

AUDEX_TEST("command: $i of old profiles still becomes standard input")
{
    const CommandScheme command = parseCommandScheme(u"oggenc -o $o $i"_s, {});
    AUDEX_CHECK(t, command.issues.isEmpty());
    AUDEX_EQUAL(t, command.arguments, QStringList({u"oggenc"_s, u"-o"_s, u"${o}"_s, u"-"_s}));
}

AUDEX_TEST("encoder command: $o is found, a value that looks like it is not")
{
    const QMap<QString, QString> album{{u"title"_s, u"$o %o"_s}};
    AUDEX_CHECK(t, hasOutputFilePlaceholder(parseCommandScheme(u"enc -o${o}.m4a -"_s, album).arguments));
    AUDEX_CHECK(t, !hasOutputFilePlaceholder({u"enc"_s, u"$$o"_s, u"%o"_s, u"${o x=1}"_s}));

    // without $o: standard input and the output file at the end, as before
    const CommandScheme command = parseCommandScheme(u"enc --title=$title"_s, album);
    AUDEX_EQUAL(t, command.arguments, QStringList({u"enc"_s, u"--title=$$o %o"_s, u"-"_s, u"${o}"_s}));
    AUDEX_EQUAL(t, substituteValues(command.arguments, {{u"o"_s, u"x.m4a"_s}}), QStringList({u"enc"_s, u"--title=$o %o"_s, u"-"_s, u"x.m4a"_s}));
}

#ifndef AUDEX_SANDBOX
AUDEX_TEST("external encoder: output file and track values are filled in")
{
    QTemporaryDir dir;
    const QString copy = dir.filePath(u"$o 100%.wav"_s);
    const QString output = dir.filePath(u"out $o %o.wav"_s);

    // tee writes standard input into both files
    const CommandScheme command = parseCommandScheme(u"tee \"$dest/${ttitle}.wav\" $o"_s, {{u"dest"_s, dir.path()}});
    AUDEX_CHECK(t, command.issues.isEmpty());
    const std::unique_ptr<AudioEncoder> encoder = ExternalEncoderFactory().create({{u"commandArgs"_s, command.arguments}});
    encoder->setTrackValues(trackValues(u"A"_s, u"$o 100%"_s, 1, QString()));

    Audex::Text error;
    const QByteArray pcm(4 * 588, '\x01');
    AUDEX_CHECK(t, encoder->open(output, 588, &error));
    AUDEX_CHECK(t, encoder->write(pcm, &error));
    AUDEX_CHECK_MSG(t, encoder->finish(&error), error.toString());

    const QByteArray expected = wavHeader(pcm.size()) + pcm;
    for (const QString &path : {output, copy}) {
        QFile file(path);
        AUDEX_CHECK_MSG(t, file.open(QIODevice::ReadOnly), path);
        AUDEX_CHECK(t, file.readAll() == expected);
    }
}

AUDEX_TEST("external encoder: no command, no output file")
{
    Audex::Text error;
    AUDEX_CHECK(t, !ExternalEncoderFactory().create({})->open(u"/tmp/x.wav"_s, 0, &error));
    AUDEX_EQUAL(t, error.toEnglish(), u"No encoder command configured."_s);
    AUDEX_CHECK(t, !ExternalEncoderFactory().create({{u"commandArgs"_s, QStringList{u"enc"_s, u"$$o"_s}}})->open(u"/tmp/x.wav"_s, 0, &error));
    AUDEX_EQUAL(t, error.toEnglish(), u"The encoder command must contain $o for the output file."_s);
    AUDEX_EQUAL(t, qint64(ExternalEncoderFactory().options().size()), qint64(1)); // the suffix
}
#endif

AUDEX_TEST("command: a program that cannot be started is named")
{
    AUDEX_EQUAL(t, missingProgram({u"sh"_s, u"-c"_s, u"true"_s}), QString());
    AUDEX_EQUAL(t, missingProgram({u"/bin/sh"_s}), QString());
    AUDEX_EQUAL(t, missingProgram({u"audex-no-such-encoder"_s, u"-"_s}), u"audex-no-such-encoder"_s);
    AUDEX_EQUAL(t, missingProgram({u"/nonexistent/encoder"_s}), u"/nonexistent/encoder"_s);
    QTemporaryDir dir;
    const QString plain = dir.filePath(u"encoder"_s);
    QFile file(plain);
    AUDEX_CHECK(t, file.open(QIODevice::WriteOnly));
    file.close();
    AUDEX_EQUAL(t, missingProgram({plain}), plain); // not executable
    AUDEX_EQUAL(t, missingProgram({}), QString());
}
