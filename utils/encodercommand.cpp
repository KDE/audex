/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "encodercommand.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

#include <optional>

using namespace Qt::StringLiterals;

namespace Audex::Encoding
{

namespace
{

namespace Var = Scheme::Var;

// a filename value: Audex embeds the cover into the files, there is no cover file
const QString CoverFile = u"cover"_s;

// Marks a placeholder in the text handed to QProcess::splitCommand(), so that
// it is never split, not even at a space between its parameters. U+FDD0 and
// U+FDD1 are noncharacters, they do not occur in a scheme.
const QChar MarkBegin(0xFDD0);
const QChar MarkEnd(0xFDD1);

using Argument = QList<Scheme::Token>;

// The scheme split into arguments with QProcess rules
QList<Argument> splitScheme(const QString &scheme, Scheme::Error *error)
{
    const Scheme::Parsed parsed = Scheme::parse(scheme);
    *error = parsed.error;
    if (parsed.error)
        return {};

    QString marked;
    QList<Scheme::Token> placeholders;
    for (const Scheme::Token &token : parsed.tokens) {
        if (token.placeholder) {
            marked += MarkBegin + QString::number(placeholders.size()) + MarkEnd;
            placeholders.append(token);
        } else {
            marked += token.text;
        }
    }

    QList<Argument> result;
    for (const QString &argument : QProcess::splitCommand(marked)) {
        Argument tokens;
        QString text;
        for (qsizetype i = 0; i < argument.size(); ++i) {
            const qsizetype end = argument.at(i) == MarkBegin ? argument.indexOf(MarkEnd, i) : -1;
            if (end > i) {
                if (!text.isEmpty())
                    tokens.append({text, std::nullopt});
                text.clear();
                tokens.append(placeholders.value(argument.sliced(i + 1, end - i - 1).toInt()));
                i = end;
            } else {
                text += argument.at(i);
            }
        }
        if (!text.isEmpty())
            tokens.append({text, std::nullopt});
        result.append(tokens);
    }
    return result;
}

void addIssue(QList<CommandIssue> *issues, CommandIssue::Kind kind, const QString &written)
{
    for (const CommandIssue &e : std::as_const(*issues))
        if (e.kind == kind && e.written == written)
            return;
    issues->append({kind, written, {}});
}

// A value becomes a whole argument or part of one. The engine reads %o as the
// output file, so a percent sign has to be doubled; a dollar sign is doubled
// as long as another step still reads the arguments.
QString escapeValue(QString value, bool escapeDollar)
{
    value.replace(u'%', u"%%"_s);
    return escapeDollar ? value.replace(u'$', u"$$"_s) : value;
}

}

QStringList trackCommandVariables()
{
    return {Var::TrackArtist, Var::TrackTitle, Var::TrackNo, Var::Isrc};
}

QMap<QString, QString> trackValues(const QString &artist, const QString &title, int trackNumber, const QString &isrc)
{
    return {{Var::TrackArtist, artist}, {Var::TrackTitle, title}, {Var::TrackNo, QString::number(trackNumber)}, {Var::Isrc, isrc}};
}

CommandScheme parseCommandScheme(const QString &scheme, const QMap<QString, QString> &values)
{
    const QStringList tracks = trackCommandVariables();
    CommandScheme result;

    Scheme::Error error;
    const QList<Argument> arguments = splitScheme(scheme, &error);
    if (error) {
        result.issues.append({CommandIssue::Kind::Syntax, QString(), error});
        return result;
    }

    for (const Argument &argument : arguments) {
        QString out;
        for (const Scheme::Token &token : argument) {
            if (!token.placeholder) {
                out += escapeValue(token.text, true); // a literal % or $ stays one
                continue;
            }
            const QString &name = token.placeholder->name;
            if (name == CoverFile) {
                addIssue(&result.issues, CommandIssue::Kind::CoverFile, token.text);
            } else if (!token.placeholder->parameters.isEmpty()) {
                // parameters are a feature of the filename schemes
                addIssue(&result.issues, CommandIssue::Kind::HasParameters, token.text);
            } else if (name == Var::InputFile) {
                out += u"-"_s; // the engine writes the WAVE data to stdin
                continue;
            } else if (name == Var::OutputFile) {
                out += u"%o"_s;
                continue;
            } else if (tracks.contains(name)) {
                out += u"${"_s + name + u'}'; // filled in when the output file is opened
                continue;
            } else if (values.contains(name)) {
                out += escapeValue(values.value(name), true);
                continue;
            } else {
                // an unknown name is a typo or a value Audex does not have
                addIssue(&result.issues, CommandIssue::Kind::UnknownName, token.text);
            }
            out += escapeValue(token.text, true);
        }
        result.arguments.append(out);
    }

    // a scheme that names no output file gets the old default: WAVE on stdin,
    // output file at the end
    if (!result.arguments.isEmpty() && !hasOutputFilePlaceholder(result.arguments))
        result.arguments << u"-"_s << u"%o"_s;
    return result;
}

bool hasOutputFilePlaceholder(const QStringList &arguments)
{
    for (const QString &argument : arguments) {
        for (qsizetype i = 0; i + 1 < argument.size(); ++i) {
            if (argument.at(i) != u'%')
                continue;
            if (argument.at(i + 1) == u'%') {
                ++i;
                continue;
            }
            if (argument.at(i + 1) == u'o')
                return true;
        }
    }
    return false;
}

QStringList substituteValues(const QStringList &arguments, const QMap<QString, QString> &values)
{
    QStringList result;
    for (const QString &argument : arguments) {
        const Scheme::Parsed parsed = Scheme::parse(argument);
        if (parsed.error) { // not written by parseCommandScheme()
            result.append(argument);
            continue;
        }
        QString out;
        for (const Scheme::Token &token : parsed.tokens) {
            if (token.placeholder && token.placeholder->parameters.isEmpty() && values.contains(token.placeholder->name))
                out += escapeValue(values.value(token.placeholder->name), false);
            else
                out += token.text;
        }
        result.append(out);
    }
    return result;
}

QList<CommandIssue> checkHookCommand(const QString &command, const QMap<QString, QString> &values)
{
    QList<CommandIssue> issues;
    Scheme::Error error;
    const QList<Argument> arguments = splitScheme(command, &error);
    if (error)
        return {{CommandIssue::Kind::Syntax, QString(), error}};

    for (const Argument &argument : arguments) {
        for (const Scheme::Token &token : argument) {
            if (!token.placeholder)
                continue;
            const QString &name = token.placeholder->name;
            if (!token.placeholder->parameters.isEmpty())
                addIssue(&issues, CommandIssue::Kind::HasParameters, token.text);
            else if (!values.contains(name) && name != Var::HookFiles && name != Var::HookOutputDir)
                addIssue(&issues, CommandIssue::Kind::UnknownName, token.text);
        }
    }
    return issues;
}

QStringList hookCommandArguments(const QString &command, const QMap<QString, QString> &albumVars, const QStringList &files, const QString &outputDir)
{
    Scheme::Error error;
    const QList<Argument> arguments = splitScheme(command, &error);

    QMap<QString, QString> values = albumVars;
    values.insert(Var::HookFiles, files.join(u' ')); // inside a larger argument it stays one argument
    values.insert(Var::HookOutputDir, outputDir);

    QStringList result;
    for (const Argument &argument : arguments) {
        const std::optional<Scheme::Placeholder> &first = argument.constFirst().placeholder;
        if (argument.size() == 1 && first && first->name == Var::HookFiles && first->parameters.isEmpty()) {
            result += files;
            continue;
        }
        QString out;
        for (const Scheme::Token &token : argument) {
            if (token.placeholder && token.placeholder->parameters.isEmpty() && values.contains(token.placeholder->name))
                out += values.value(token.placeholder->name);
            else
                out += token.text;
        }
        result += out;
    }
    return result;
}

QList<EncoderPreset> encoderPresets()
{
    QList<EncoderPreset> presets;
    QStringList names;
    const QStringList files = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, u"encoderpresets.json"_s, QStandardPaths::LocateFile);
    if (files.isEmpty()) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            qWarning().noquote() << "encoderpresets.json was not found in" << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).join(u", "_s)
                                 << "- is Audex installed, and is its data directory in XDG_DATA_DIRS? No encoder presets are offered.";
        }
    }
    for (const QString &fileName : files) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        for (const QJsonValue &value : doc.array()) {
            const QJsonObject o = value.toObject();
            const QString name = o.value("name"_L1).toString();
            const QString command = o.value("command"_L1).toString();
            if (name.isEmpty() || command.isEmpty() || names.contains(name))
                continue;
            names << name;
            presets.append({name, command, o.value("suffix"_L1).toString()});
        }
    }
    return presets;
}

QString commandToString(const QStringList &arguments)
{
    QStringList parts;
    for (const QString &argument : arguments) {
        if (!argument.isEmpty() && !argument.contains(u' ') && !argument.contains(u'\t') && !argument.contains(u'"')) {
            parts << argument;
            continue;
        }
        QString quoted = argument;
        parts << u'"' + quoted.replace(u'"', u"\"\"\""_s) + u'"'; // as QProcess::splitCommand() reads it
    }
    return parts.join(u' ');
}

}
