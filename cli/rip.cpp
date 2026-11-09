/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "commands.h"
#include "common.h"

#include "encoding/registry.h"
#include "utils/cuesheetwriter.h"
#include "utils/ripjob.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QTimer>

#include <algorithm>
#include <unistd.h>

using namespace Qt::StringLiterals;

namespace Audex::Cli
{

namespace
{

struct Tr {
    Q_DECLARE_TR_FUNCTIONS(Audex::Cli)
};

QString accurateRipOffWarning()
{
    return Tr::tr("AccurateRip is turned off (--no-accuraterip): nothing confirms that this rip is correct.");
}

// "1,2,5-7,9", "3-" (to the last), "-4" (from track 1); 0 is the hidden
// track. Data tracks inside a range are left out, named alone they are an error.
std::optional<QList<int>> parseTrackList(const QString &text, const Cdda::Toc &toc, bool hiddenTrack, QString *error)
{
    QList<int> all;
    if (hiddenTrack)
        all << 0;
    for (const Cdda::Track &t : toc.tracks)
        all << t.number;
    const auto isAudio = [&](int number) {
        if (number == 0)
            return hiddenTrack;
        const Cdda::Track *t = toc.track(number);
        return t && t->audio;
    };
    const auto number = [&](const QString &s, int fallback, int *value) {
        if (s.trimmed().isEmpty()) {
            *value = fallback;
            return true;
        }
        bool ok = false;
        *value = s.trimmed().toInt(&ok);
        return ok;
    };

    QList<int> result;
    for (const QString &part : text.split(u',')) {
        const QString item = part.trimmed();
        if (item.isEmpty()) {
            *error = Tr::tr("Empty entry in the track list \"%1\".").arg(text);
            return std::nullopt;
        }
        const qsizetype dash = item.indexOf(u'-');
        int first = 0;
        int last = 0;
        if (dash < 0) {
            if (!number(item, -1, &first)) {
                *error = Tr::tr("\"%1\" is not a track number.").arg(item);
                return std::nullopt;
            }
            if (!all.contains(first)) {
                *error = first == 0 ? Tr::tr("The disc has no hidden track (0).") : Tr::tr("The disc has no track %1.").arg(first);
                return std::nullopt;
            }
            if (!isAudio(first)) {
                *error = Tr::tr("Track %1 is a data track.").arg(first);
                return std::nullopt;
            }
            result << first;
            continue;
        }
        // an open start is the first track: the hidden track only when named
        if (!number(item.left(dash), toc.tracks.first().number, &first) || !number(item.mid(dash + 1), all.last(), &last) || first > last) {
            *error = Tr::tr("\"%1\" is not a range of tracks.").arg(item);
            return std::nullopt;
        }
        if (!all.contains(first) || !all.contains(last)) {
            *error = Tr::tr("The range %1 goes beyond the tracks of the disc.").arg(item);
            return std::nullopt;
        }
        for (int n : std::as_const(all))
            if (n >= first && n <= last && isAudio(n))
                result << n;
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    if (result.isEmpty()) {
        *error = Tr::tr("The track list \"%1\" contains no audio track.").arg(text);
        return std::nullopt;
    }
    return result;
}

bool isTerminal(FILE *stream)
{
    return ::isatty(::fileno(stream)) == 1;
}

}

int rip(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        Tr::tr("Rips the disc as WAVE files, one per track, with a cue sheet and a rip log, and verifies them with AccurateRip. "
               "Pregaps, indexes, ISRCs and the catalog number are read from the Q sub-channel."));
    parser.addHelpOption();
    parser.addPositionalArgument(u"rip"_s, Tr::tr("The command."));
    const QCommandLineOption device = deviceOption();
    const QCommandLineOption tracks({u"t"_s, u"tracks"_s},
                                    Tr::tr("Tracks to rip, e.g. 1,2,5-7,9 or 3- (default: all; 0 is the hidden track)."),
                                    Tr::tr("list", "value of --tracks"));
    const QCommandLineOption output({u"o"_s, u"output"_s},
                                    Tr::tr("Folder for the files (default: the current one)."),
                                    Tr::tr("folder", "value of --output"),
                                    u"."_s);
    const QCommandLineOption offset(u"offset"_s,
                                    Tr::tr("Read offset of the drive in samples (default: the one \"audex-cli offset\" saved for the drive model)."),
                                    Tr::tr("samples", "value of --offset"));
    const QCommandLineOption noAccurateRip(u"no-accuraterip"_s, Tr::tr("Do not verify with AccurateRip (not recommended)."));
    const QCommandLineOption fast(u"fast"_s, Tr::tr("Read every sector once instead of securely."));
    const QCommandLineOption c2(u"c2"_s, Tr::tr("Use the C2 error pointers of the drive (only if they are reliable)."));
    const QCommandLineOption overread(u"overread"_s, Tr::tr("Read into the lead-in and lead-out (if the drive can)."));
    const QCommandLineOption speed(u"speed"_s, Tr::tr("Read speed (default: the maximum)."), u"x"_s, u"0"_s);
    const QCommandLineOption pregapsWithTrack(u"pregaps-with-track"_s,
                                              Tr::tr("Put each pregap at the start of its own track's file (default: at the end of the previous one)."));
    const QCommandLineOption cdg(u"cdg"_s, Tr::tr("Save CD+G graphics as .cdg files next to the tracks, if the disc has them."));
    const QCommandLineOption overwrite(u"overwrite"_s, Tr::tr("Overwrite existing files."));
    const QCommandLineOption quiet({u"q"_s, u"quiet"_s}, Tr::tr("No progress and no messages, only warnings and the result."));
    parser.addOptions({device, tracks, output, offset, noAccurateRip, fast, c2, overread, speed, pregapsWithTrack, cdg, overwrite, quiet});
    parser.process(arguments);

    bool ok = false;
    const int readSpeed = parser.value(speed).toInt(&ok);
    if (!ok || readSpeed < 0) {
        print(stderr, Tr::tr("--speed needs a number (0 = the maximum)."));
        return ExitUsage;
    }
    std::optional<int> readOffset;
    if (parser.isSet(offset)) {
        readOffset = parser.value(offset).toInt(&ok);
        if (!ok) {
            print(stderr, Tr::tr("--offset needs a number of samples, e.g. +6 or -1164."));
            return ExitUsage;
        }
    }
    const bool talk = !parser.isSet(quiet);
    const bool accurateRip = !parser.isSet(noAccurateRip);

    installCancelHandler();
    DriveEntry drive;
    DriveLock lock;
    DiscReadResult disc;
    if (const int code = openDisc(parser.value(device), &drive, &lock, &disc); code != ExitOk)
        return code;
    const Cdda::Toc toc = disc.info.toc();
    if (toc.audioTrackNumbers().isEmpty()) {
        print(stderr, Tr::tr("The disc has no audio tracks."));
        return ExitError;
    }

    // the read offset: given, or measured before for the drive model
    if (!readOffset && !disc.driveModel.isEmpty()) {
        if (const std::optional<OffsetStore::Entry> saved = OffsetStore::load().find(disc.driveModel)) {
            readOffset = saved->offset;
            if (talk)
                print(stderr, Tr::tr("Read offset %1 samples, saved for \"%2\".").arg(signedNumber(saved->offset), disc.driveModel));
        }
    }
    if (!readOffset) {
        print(stderr,
              Tr::tr("The read offset of this drive is not known. Measure it with \"audex-cli offset\" (saved for the drive model) or pass it with --offset."));
        return ExitUsage;
    }
    if (parser.isSet(c2) && disc.c2Known && !disc.c2Capable) {
        print(stderr, Tr::tr("The drive reports no C2 error pointers; --c2 cannot be used."));
        return ExitUsage;
    }

    // the tracks: all audio, the hidden track only if it is not silent
    const bool hiddenTrack = toc.htoaSectorCount() > 0;
    QList<int> selected;
    if (parser.isSet(tracks)) {
        QString error;
        const std::optional<QList<int>> parsed = parseTrackList(parser.value(tracks), toc, hiddenTrack, &error);
        if (!parsed) {
            print(stderr, error);
            return ExitUsage;
        }
        selected = *parsed;
    } else {
        if (hiddenTrack && !disc.htoaSilent)
            selected << 0;
        selected << toc.audioTrackNumbers();
    }

    // fixed names: 01.wav ... (00.wav: the hidden track), disc.cue, disc.log
    const QDir dir(parser.value(output));
    if (!QDir().mkpath(dir.absolutePath())) {
        print(stderr, Tr::tr("Cannot create the folder %1.").arg(dir.absolutePath()));
        return ExitError;
    }
    QMap<int, QString> filePaths;
    for (int number : std::as_const(selected))
        filePaths.insert(number, dir.absoluteFilePath(u"%1.wav"_s.arg(number, 2, 10, QLatin1Char('0'))));
    const QString cuePath = dir.absoluteFilePath(u"disc.cue"_s);
    const QString logPath = dir.absoluteFilePath(u"disc.log"_s);
    if (!parser.isSet(overwrite)) {
        QStringList existing;
        QStringList paths = QStringList(filePaths.values()) << cuePath << logPath;
        if (parser.isSet(cdg))
            for (const QString &path : filePaths)
                paths << path.chopped(4) + u".cdg"_s; // next to the track: 01.cdg
        for (const QString &path : paths)
            if (QFile::exists(path))
                existing << QDir::toNativeSeparators(path);
        if (!existing.isEmpty()) {
            print(stderr, Tr::tr("These files exist already (--overwrite replaces them):\n  %1").arg(existing.join(u"\n  "_s)));
            return ExitError;
        }
    }

    auto encoders = std::make_shared<Encoding::EncoderRegistry>(); // WAVE is built in

    RipRequest rq;
    rq.drive = drive;
    rq.disc = disc.info;
    rq.tracks = selected;
    rq.options.mode = parser.isSet(fast) ? Rip::ReadMode::Fast : Rip::ReadMode::Secure;
    rq.options.readOffset = *readOffset;
    rq.options.overread = parser.isSet(overread);
    rq.options.useC2 = parser.isSet(c2);
    rq.options.cacheDefeat = true;
    rq.options.cacheDefeatReads = 0; // measured by this rip
    rq.options.readSpeed = readSpeed;
    if (drive.isSimulated())
        rq.options.cacheDefeatDistance = 1500; // the demo disc is short
    rq.outputDirectory = dir.absolutePath();
    rq.application = u"audex-cli "_s + QCoreApplication::applicationVersion();
    rq.encoders = encoders;
    rq.encoderId = u"wav"_s;
    rq.writeTags = false;
    rq.embedCover = false;
    rq.accurateRipLookup = accurateRip;
    if (!accurateRip)
        rq.accurateRipSkipped = Text::translate("Audex::Cli", "turned off with --no-accuraterip; nothing confirms that this rip is correct");
    rq.keepConfirmedTracks = accurateRip && rq.options.mode == Rip::ReadMode::Secure;
    rq.scanSubchannel = true;
    rq.scanIsrcMcn = true;
    rq.pregapsWithTrack = parser.isSet(pregapsWithTrack);
    rq.readCdg = parser.isSet(cdg); // the rip checks first whether the disc has graphics
    if (const std::optional<bool> stream = reportedAccurateStream(drive.id); stream && !*stream)
        rq.inaccurateStream = true;
    rq.logFilePath = logPath;
    rq.filePaths = filePaths;

    if (!accurateRip)
        print(stderr, Tr::tr("WARNING: %1").arg(accurateRipOffWarning()));
    if (talk)
        print(stderr,
              (rq.options.mode == Rip::ReadMode::Secure ? Tr::tr("Ripping %n track(s) from %1 to %2 (secure)...", nullptr, int(selected.size()))
                                                        : Tr::tr("Ripping %n track(s) from %1 to %2 (fast)...", nullptr, int(selected.size())))
                  .arg(disc.driveName, QDir::toNativeSeparators(dir.absolutePath())));

    // --- the rip ---
    const bool progressLine = talk && isTerminal(stderr);
    bool progressShown = false;
    const auto clearProgress = [&] {
        if (progressShown) {
            std::fputs("\r\033[K", stderr);
            progressShown = false;
        }
    };

    RipJob job(rq);
    RipSummary summary;
    QEventLoop loop;
    QObject::connect(&job, &RipJob::progress, &loop, [&](qint64 done, qint64 total, int trackNumber, qint64) {
        if (!progressLine || total <= 0)
            return;
        std::fputs((u"\r\033[K"_s + Tr::tr("Track %1 of %2, %3 %").arg(selected.indexOf(trackNumber) + 1).arg(selected.size()).arg(int(done * 100 / total)))
                       .toLocal8Bit()
                       .constData(),
                   stderr);
        std::fflush(stderr);
        progressShown = true;
    });
    QObject::connect(&job, &RipJob::cdgProgress, &loop, [&](int pass, qint64 done, qint64 total, int, qint64) {
        if (!progressLine || total <= 0)
            return;
        std::fputs((u"\r\033[K"_s + Tr::tr("CD+G graphics, pass %1, %2 %").arg(pass).arg(int(done * 100 / total))).toLocal8Bit().constData(), stderr);
        std::fflush(stderr);
        progressShown = true;
    });
    QObject::connect(&job, &RipJob::message, &loop, [&](int level, const QString &text) {
        if (level >= int(Rip::LogLevel::Warning)) {
            clearProgress();
            print(stderr, (level == int(Rip::LogLevel::Error) ? Tr::tr("Error: %1") : Tr::tr("Warning: %1")).arg(text));
        } else if (talk && level == int(Rip::LogLevel::Info)) {
            clearProgress();
            print(stderr, text);
        }
    });
    QObject::connect(&job, &RipJob::finished, &loop, [&](const RipSummary &s) {
        summary = s;
        loop.quit();
    });
    QTimer cancelCheck;
    QObject::connect(&cancelCheck, &QTimer::timeout, &loop, [&] {
        if (canceled())
            job.cancel();
    });
    cancelCheck.start(100);
    job.start();
    loop.exec();
    clearProgress();

    if (summary.canceled) {
        print(stderr, Tr::tr("Canceled. Finished tracks are kept; the log tells what was done."));
        return ExitCanceled;
    }
    if (!summary.completed) {
        print(stderr, Tr::tr("The rip failed: %1").arg(summary.error));
        return ExitError;
    }

    // --- cue sheet: the written files, relative to it ---
    CueSheetWriter writer(disc.info);
    writer.setSubchannel(summary.subchannel);
    QList<int> written;
    QStringList names;
    for (int number : std::as_const(selected)) {
        if (summary.files.contains(filePaths.value(number))) {
            written << number;
            names << dir.relativeFilePath(filePaths.value(number));
        }
    }
    const auto pregaps = summary.pregapsWithTrack ? CueSheetWriter::Pregaps::WithOwnTrack : CueSheetWriter::Pregaps::AppendedToPrevious;
    QFile cue(cuePath);
    const bool cueWritten = cue.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)
        && cue.write(writer.cueSheet(names, written, true, true, pregaps).join(u'\n').toUtf8() + '\n') > 0;
    if (!cueWritten)
        print(stderr, Tr::tr("Warning: cannot write %1: %2").arg(cuePath, cue.errorString()));

    // --- result ---
    const int checkable = int(std::count_if(selected.cbegin(), selected.cend(), [](int n) {
        return n != 0; // AccurateRip does not cover the hidden track
    }));
    QStringList out;
    out << field(Tr::tr("Tracks"), Tr::tr("%n written to %1", "tracks", int(summary.files.size())).arg(QDir::toNativeSeparators(dir.absolutePath())));
    out << field(Tr::tr("Cue sheet"),
                 summary.subchannel.supported ? QDir::toNativeSeparators(cuePath)
                                              : Tr::tr("%1 (no pregaps: the drive gave no Q sub-channel)").arg(QDir::toNativeSeparators(cuePath)));
    if (parser.isSet(cdg))
        out << field(u"CD+G"_s,
                     summary.cdgFiles.isEmpty() ? Tr::tr("no graphics on the disc, no .cdg files")
                                                : Tr::tr("%n .cdg file(s) next to the tracks", nullptr, int(summary.cdgFiles.size())));
    out << field(Tr::tr("Log"), summary.logFile.isEmpty() ? Tr::tr("not written") : QDir::toNativeSeparators(summary.logFile));
    out << field(Tr::tr("Reading"),
                 summary.suspiciousPositions > 0 ? Tr::tr("%n suspicious position(s), see the log", nullptr, summary.suspiciousPositions)
                                                 : Tr::tr("no errors"));
    int code = ExitOk;
    if (!accurateRip) {
        out << field(u"AccurateRip"_s, Tr::tr("NOT CHECKED (--no-accuraterip)"));
        code = ExitUnconfirmed;
    } else if (!summary.accurateRipError.isEmpty()) {
        out << field(u"AccurateRip"_s, Tr::tr("not checked, the lookup failed (%1)").arg(summary.accurateRipError));
        code = ExitUnconfirmed;
    } else if (summary.accurateRipMismatches > 0) {
        out << field(u"AccurateRip"_s, Tr::tr("%n track(s) NOT accurate", nullptr, summary.accurateRipMismatches));
        code = ExitFailed;
    } else if (summary.accurateRipConfirmed >= checkable && !selected.contains(0)) {
        out << field(u"AccurateRip"_s, Tr::tr("all %n track(s) accurately ripped", nullptr, checkable));
    } else if (summary.accurateRipConfirmed >= checkable) {
        out << field(u"AccurateRip"_s, Tr::tr("all %n track(s) accurately ripped; the hidden track is not in the database (it never is)", nullptr, checkable));
        code = ExitUnconfirmed;
    } else {
        out << field(u"AccurateRip"_s,
                     Tr::tr("%1 of %n track(s) confirmed, the others are not in the database", nullptr, checkable).arg(summary.accurateRipConfirmed));
        code = ExitUnconfirmed;
    }
    if (summary.suspiciousPositions > 0)
        code = ExitFailed;
    if (!cueWritten || summary.logFile.isEmpty())
        code = code == ExitFailed ? ExitFailed : ExitError;
    print(stdout, out.join(u'\n'));
    if (!accurateRip)
        print(stderr, Tr::tr("WARNING: %1 \"audex-cli verify %2\" can check it later.").arg(accurateRipOffWarning(), QDir::toNativeSeparators(logPath)));
    return code;
}

}
