/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// audex-cli: Audex without the user interface. Only the engine (Qt, no KDE
// Frameworks), so it runs on servers and in scripts.

#include "audex-version.h"
#include "commands.h"
#include "common.h"
#include "encoding/registry.h"
#include "utils/ripverify.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

using namespace Qt::StringLiterals;

namespace Audex::Cli
{

namespace
{

struct Tr {
    Q_DECLARE_TR_FUNCTIONS(Audex::Cli)
};

QString usage()
{
    const auto command = [](const char *name, const QString &description) {
        return u"  %1 %2"_s.arg(QLatin1String(name), -8).arg(description);
    };
    return QStringList{Tr::tr("Usage: audex-cli <command> [options]"),
                       QString(),
                       Tr::tr("Commands:"),
                       command("info", Tr::tr("Show information about the drive and the inserted disc")),
                       command("offset", Tr::tr("Measure the read offset of the drive with AccurateRip and save it")),
                       command("rip", Tr::tr("Rip the disc as WAV files with cue sheet and log, verified with AccurateRip")),
                       command("verify", Tr::tr("Check finished rips against their rip log, AccurateRip and the CUETools database")),
                       QString(),
                       Tr::tr("Use 'audex-cli <command> --help' to show options for a specific command.")}
        .join(u'\n');
}

int exitCode(Verify::Verdict verdict)
{
    switch (verdict) {
    case Verify::Verdict::Ok:
        return ExitOk;
    case Verify::Verdict::Unconfirmed:
        return ExitUnconfirmed;
    case Verify::Verdict::Failed:
        return ExitFailed;
    case Verify::Verdict::Error:
        break;
    }
    return ExitError;
}

// several logs: the worst result counts
int severity(int code)
{
    switch (code) {
    case ExitOk:
        return 0;
    case ExitUnconfirmed:
        return 1;
    case ExitError:
        return 2;
    default:
        return 3; // failed
    }
}

QJsonObject toJson(const QString &logPath, const Verify::Result &r)
{
    QJsonArray tracks;
    for (const Verify::TrackResult &t : r.tracks) {
        QJsonObject o{{u"number"_s, t.number}, {u"file"_s, t.path}, {u"changed"_s, t.changed()}};
        if (t.crc)
            o.insert(u"crc"_s, hex8(*t.crc));
        if (t.loggedCrc)
            o.insert(u"loggedCrc"_s, hex8(*t.loggedCrc));
        if (t.accurateRipConfidence >= 0)
            o.insert(u"accurateRipConfidence"_s, t.accurateRipConfidence);
        if (t.ctdbConfidence >= 0)
            o.insert(u"ctdbConfidence"_s, t.ctdbConfidence);
        if (!t.problem.isEmpty())
            o.insert(u"problem"_s, t.problem);
        tracks.append(o);
    }
    QJsonObject o{{u"log"_s, logPath}, {u"verdict"_s, Verify::verdictName(r.verdict)}, {u"application"_s, r.application}, {u"tracks"_s, tracks}};
    if (!r.error.isEmpty())
        o.insert(u"error"_s, r.error);
    if (r.accurateRipAsked)
        o.insert(u"accurateRip"_s, QJsonObject{{u"known"_s, r.accurateRipKnown}, {u"mismatches"_s, r.accurateRipMismatches}});
    if (r.ctdbAsked)
        o.insert(u"ctdb"_s, QJsonObject{{u"known"_s, r.ctdbKnown}, {u"mismatches"_s, r.ctdbMismatches}});
    return o;
}

}

int verify(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        Tr::tr("Checks finished rips: the files are read back and compared with the copy CRCs "
               "of the rip log. AccurateRip and the CUETools database are checked again."));
    parser.addHelpOption();
    parser.addPositionalArgument(u"verify"_s, Tr::tr("The command."));
    parser.addPositionalArgument(u"log"_s, Tr::tr("Rip log(s) of Audex."), u"log..."_s);
    const QCommandLineOption offline(u"offline"_s, Tr::tr("Only compare the files with the log without querying online databases."));
    const QCommandLineOption directory(u"dir"_s, Tr::tr("Look for the files in <folder> (one log only)."), u"folder"_s);
    const QCommandLineOption confidence(u"min-confidence"_s, Tr::tr("Minimum database confidence required for a track (default: 2)."), u"n"_s, u"2"_s);
    const QCommandLineOption json(u"json"_s, Tr::tr("Print the results as JSON."));
    const QCommandLineOption quiet(u"quiet"_s, Tr::tr("Print only the result of each log."));
    parser.addOptions({offline, directory, confidence, json, quiet});
    parser.process(arguments);

    const QStringList logs = parser.positionalArguments().mid(1);
    bool numberOk = false;
    const int minConfidence = parser.value(confidence).toInt(&numberOk);
    if (logs.isEmpty() || !numberOk || minConfidence < 1 || (parser.isSet(directory) && logs.size() > 1)) {
        print(stderr, parser.helpText());
        return ExitUsage;
    }

    auto encoders = std::make_shared<Encoding::EncoderRegistry>();
    encoders->loadPlugins(); // FLAC

    int worst = ExitOk;
    QJsonArray results;
    for (const QString &log : logs) {
        Verify::Request rq;
        rq.logPath = log;
        rq.directory = parser.value(directory);
        rq.encoders = encoders;
        rq.online = !parser.isSet(offline);
        rq.minConfidence = minConfidence;
        const bool talk = !parser.isSet(json) && !parser.isSet(quiet);
        const Verify::Result r = Verify::verifyRip(rq, [talk](const QString &text) {
            if (talk)
                print(stderr, text);
        });

        const int code = exitCode(r.verdict);
        if (severity(code) > severity(worst))
            worst = code;
        if (parser.isSet(json))
            results.append(toJson(log, r));
        else if (parser.isSet(quiet))
            print(stdout, u"%1: %2%3"_s.arg(log, Verify::verdictName(r.verdict), r.error.isEmpty() ? QString() : u" (%1)"_s.arg(r.error)));
        else
            print(stdout, (r.report.isEmpty() ? QStringList{log + u": "_s + r.error} : r.report).join(u'\n') + u'\n');
    }
    if (parser.isSet(json))
        print(stdout, QString::fromUtf8(QJsonDocument(results).toJson()));
    return worst;
}

}

int main(int argc, char **argv)
{
    using namespace Audex::Cli;
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"audex-cli"_s);
    QCoreApplication::setApplicationVersion(QStringLiteral(AUDEX_VERSION_STRING));
    // the texts of QCommandLineParser itself ("Usage:", "Options:", ...)
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(), u"qtbase"_s, u"_"_s, QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QCoreApplication::installTranslator(&qtTranslator);

    const QStringList arguments = app.arguments();
    const QString command = arguments.value(1);
    if (command == u"verify"_s)
        return verify(arguments);
    if (command == u"info"_s)
        return info(arguments);
    if (command == u"offset"_s)
        return offset(arguments);
    if (command == u"rip"_s)
        return rip(arguments);
    if (command == u"--version"_s || command == u"-v"_s) {
        print(stdout, u"audex-cli "_s + QCoreApplication::applicationVersion());
        return ExitOk;
    }
    print(command == u"--help"_s || command == u"-h"_s ? stdout : stderr, usage());
    return command == u"--help"_s || command == u"-h"_s ? ExitOk : ExitUsage;
}
