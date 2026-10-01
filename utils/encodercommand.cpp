/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "encodercommand.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

#include <functional>
#include <optional>

using namespace Qt::StringLiterals;

namespace Audex::Encoding
{

namespace
{

// names of utils/schemeparser.h that only make sense in a command
const QString InputFile = u"i"_s;
const QString OutputFile = u"o"_s;

QString readName(const QString &argument, qsizetype &at, bool *braced, bool *hasParameters)
{
    qsizetype i = at + 1; // behind the dollar sign
    *braced = i < argument.size() && argument.at(i) == u'{';
    *hasParameters = false;
    if (*braced)
        ++i;
    const qsizetype start = i;
    while (i < argument.size() && (argument.at(i).isLetterOrNumber() || argument.at(i) == u'_'))
        ++i;
    const QString name = argument.sliced(start, i - start);
    if (*braced) {
        if (name.isEmpty())
            return {}; // not a placeholder after all
        const qsizetype end = argument.indexOf(u'}', i);
        // no closing brace: the scheme was split at a space inside the braces,
        // which only parameters have
        *hasParameters = end < 0 || end > i;
        i = end < 0 ? argument.size() : end + 1;
    }
    at = i;
    return name;
}

// A value becomes a whole argument or part of one. The engine reads %o as the
// output file, so a percent sign has to be doubled; a dollar sign is doubled
// as long as another step still reads the arguments.
QString escapeValue(QString value, bool escapeDollar)
{
    value.replace(u'%', u"%%"_s);
    return escapeDollar ? value.replace(u'$', u"$$"_s) : value;
}

// Walks one argument. `resolve` returns the replacement for a placeholder, or
// nothing to leave it as it was written. The last step turns $$ into $.
QString substituteArgument(const QString &argument,
                           bool lastStep,
                           const std::function<std::optional<QString>(const QString &name, bool parameters, const QString &written)> &resolve)
{
    QString out;
    for (qsizetype i = 0; i < argument.size();) {
        if (argument.at(i) != u'$') {
            out += argument.at(i++);
            continue;
        }
        if (i + 1 < argument.size() && argument.at(i + 1) == u'$') { // $$ is a dollar sign
            out += lastStep ? u"$"_s : u"$$"_s;
            i += 2;
            continue;
        }
        qsizetype next = i;
        bool braced = false;
        bool parameters = false;
        const QString name = readName(argument, next, &braced, &parameters);
        if (name.isEmpty()) {
            out += argument.at(i++);
            continue;
        }
        const QString written = braced ? u"${%1%2}"_s.arg(name, parameters ? u" ..."_s : QString()) : u'$' + name;
        out += resolve(name, parameters, written).value_or(written);
        i = next;
    }
    return out;
}

}

QStringList unsupportedCommandVariables()
{
    return {u"cover"_s}; // the cover is embedded by Audex, there is no file
}

QStringList trackCommandVariables()
{
    return {u"tartist"_s, u"ttitle"_s, u"trackno"_s, u"isrc"_s};
}

QMap<QString, QString> trackValues(const QString &artist, const QString &title, int trackNumber, const QString &isrc)
{
    return {{u"tartist"_s, artist}, {u"ttitle"_s, title}, {u"trackno"_s, QString::number(trackNumber)}, {u"isrc"_s, isrc}};
}

CommandScheme parseCommandScheme(const QString &scheme, const QMap<QString, QString> &values)
{
    const QStringList tracks = trackCommandVariables();
    CommandScheme result;

    const auto resolve = [&](const QString &name, bool parameters, const QString &written) -> std::optional<QString> {
        if (parameters || (!values.contains(name) && !tracks.contains(name) && name != InputFile && name != OutputFile)) {
            // placeholders with parameters need the old scheme parser, and an
            // unknown name is a typo or a value Audex does not have
            if (!result.unsupported.contains(written))
                result.unsupported.append(written);
            return std::nullopt;
        }
        if (name == InputFile)
            return u"-"_s; // the engine writes the WAVE data to stdin
        if (name == OutputFile)
            return u"%o"_s;
        if (tracks.contains(name))
            return std::nullopt; // filled in when the output file is opened
        return escapeValue(values.value(name), true);
    };

    for (const QString &argument : QProcess::splitCommand(scheme))
        result.arguments.append(substituteArgument(argument, false, resolve));

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
    const auto resolve = [&values](const QString &name, bool parameters, const QString &) -> std::optional<QString> {
        if (parameters || !values.contains(name))
            return std::nullopt;
        return escapeValue(values.value(name), false);
    };

    QStringList result;
    for (const QString &argument : arguments)
        result.append(substituteArgument(argument, true, resolve));
    return result;
}

QStringList hookCommandArguments(const QString &command, const QMap<QString, QString> &albumVars, const QStringList &files, const QString &outputDir)
{
    QStringList result;
    for (const QString &argument : substituteValues(QProcess::splitCommand(command), albumVars)) {
        if (argument == u"%f"_s) {
            result += files;
            continue;
        }
        QString out;
        for (qsizetype i = 0; i < argument.size(); ++i) {
            if (argument.at(i) == u'%' && i + 1 < argument.size()) {
                const QChar next = argument.at(i + 1);
                if (next == u'%') {
                    out += u'%';
                    ++i;
                    continue;
                }
                if (next == u'd') {
                    out += outputDir;
                    ++i;
                    continue;
                }
                if (next == u'f') {
                    out += files.join(u' '); // inside a larger argument it stays one argument
                    ++i;
                    continue;
                }
            }
            out += argument.at(i);
        }
        result += out;
    }
    return result;
}

QList<EncoderPreset> encoderPresets()
{
    QList<EncoderPreset> presets;
    QStringList names;
    QStringList files = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, u"encoderpresets.json"_s, QStandardPaths::LocateFile);
    files << u":/encoderpresets.json"_s;
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
