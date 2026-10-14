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

const QString AccurateRipOffWarning = u"AccurateRip is turned off (--no-accuraterip): nothing confirms that this rip is correct."_s;

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
            *error = u"Empty entry in the track list \"%1\"."_s.arg(text);
            return std::nullopt;
        }
        const qsizetype dash = item.indexOf(u'-');
        int first = 0;
        int last = 0;
        if (dash < 0) {
            if (!number(item, -1, &first)) {
                *error = u"\"%1\" is not a track number."_s.arg(item);
                return std::nullopt;
            }
            if (!all.contains(first)) {
                *error = first == 0 ? u"The disc has no hidden track (0)."_s : u"The disc has no track %1."_s.arg(first);
                return std::nullopt;
            }
            if (!isAudio(first)) {
                *error = u"Track %1 is a data track."_s.arg(first);
                return std::nullopt;
            }
            result << first;
            continue;
        }
        // an open start is the first track: the hidden track only when named
        if (!number(item.left(dash), toc.tracks.first().number, &first) || !number(item.mid(dash + 1), all.last(), &last) || first > last) {
            *error = u"\"%1\" is not a range of tracks."_s.arg(item);
            return std::nullopt;
        }
        if (!all.contains(first) || !all.contains(last)) {
            *error = u"The range %1 goes beyond the tracks of the disc."_s.arg(item);
            return std::nullopt;
        }
        for (int n : std::as_const(all))
            if (n >= first && n <= last && isAudio(n))
                result << n;
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    if (result.isEmpty()) {
        *error = u"The track list \"%1\" contains no audio track."_s.arg(text);
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
        u"Rips the disc as WAVE files, one per track, with a cue sheet and a rip log, and verifies them with AccurateRip. "
        "Pregaps, indexes, ISRCs and the catalog number are read from the Q sub-channel."_s);
    parser.addHelpOption();
    parser.addPositionalArgument(u"rip"_s, u"The command."_s);
    const QCommandLineOption device = deviceOption();
    const QCommandLineOption tracks({u"t"_s, u"tracks"_s}, u"Tracks to rip, e.g. 1,2,5-7,9 or 3- (default: all; 0 is the hidden track)."_s, u"list"_s);
    const QCommandLineOption output({u"o"_s, u"output"_s}, u"Folder for the files (default: the current one)."_s, u"folder"_s, u"."_s);
    const QCommandLineOption offset(u"offset"_s,
                                    u"Read offset of the drive in samples (default: the one \"audex-cli offset\" saved for the drive model)."_s,
                                    u"samples"_s);
    const QCommandLineOption noAccurateRip(u"no-accuraterip"_s, u"Do not verify with AccurateRip (not recommended)."_s);
    const QCommandLineOption fast(u"fast"_s, u"Read every sector once instead of securely."_s);
    const QCommandLineOption c2(u"c2"_s, u"Use the C2 error pointers of the drive (only if they are reliable)."_s);
    const QCommandLineOption overread(u"overread"_s, u"Read into the lead-in and lead-out (if the drive can)."_s);
    const QCommandLineOption speed(u"speed"_s, u"Read speed (default: the maximum)."_s, u"x"_s, u"0"_s);
    const QCommandLineOption pregapsWithTrack(u"pregaps-with-track"_s,
                                              u"Put each pregap at the start of its own track's file (default: at the end of the previous one)."_s);
    const QCommandLineOption cdg(u"cdg"_s, u"Save CD+G graphics as .cdg files next to the tracks, if the disc has them."_s);
    const QCommandLineOption overwrite(u"overwrite"_s, u"Overwrite existing files."_s);
    const QCommandLineOption quiet({u"q"_s, u"quiet"_s}, u"No progress and no messages, only warnings and the result."_s);
    parser.addOptions({device, tracks, output, offset, noAccurateRip, fast, c2, overread, speed, pregapsWithTrack, cdg, overwrite, quiet});
    parser.process(arguments);

    bool ok = false;
    const int readSpeed = parser.value(speed).toInt(&ok);
    if (!ok || readSpeed < 0) {
        print(stderr, u"--speed needs a number (0 = the maximum)."_s);
        return ExitUsage;
    }
    std::optional<int> readOffset;
    if (parser.isSet(offset)) {
        readOffset = parser.value(offset).toInt(&ok);
        if (!ok) {
            print(stderr, u"--offset needs a number of samples, e.g. +6 or -1164."_s);
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
        print(stderr, u"The disc has no audio tracks."_s);
        return ExitError;
    }

    // the read offset: given, or measured before for the drive model
    if (!readOffset && !disc.driveModel.isEmpty()) {
        if (const std::optional<OffsetStore::Entry> saved = OffsetStore::load().find(disc.driveModel)) {
            readOffset = saved->offset;
            if (talk)
                print(stderr, u"Read offset %1 samples, saved for \"%2\"."_s.arg(signedNumber(saved->offset), disc.driveModel));
        }
    }
    if (!readOffset) {
        print(stderr,
              u"The read offset of this drive is not known. Measure it with \"audex-cli offset\" (saved for the drive model) or pass it with --offset."_s);
        return ExitUsage;
    }
    if (parser.isSet(c2) && disc.c2Known && !disc.c2Capable) {
        print(stderr, u"The drive reports no C2 error pointers; --c2 cannot be used."_s);
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
        print(stderr, u"Cannot create the folder %1."_s.arg(dir.absolutePath()));
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
            print(stderr, u"These files exist already (--overwrite replaces them):\n  %1"_s.arg(existing.join(u"\n  "_s)));
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
        rq.accurateRipSkipped = u"turned off with --no-accuraterip; nothing confirms that this rip is correct"_s;
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
        print(stderr, u"WARNING: "_s + AccurateRipOffWarning);
    if (talk)
        print(stderr,
              u"Ripping %1 track(s) from %2 to %3 (%4)..."_s.arg(selected.size())
                  .arg(disc.driveName, QDir::toNativeSeparators(dir.absolutePath()), rq.options.mode == Rip::ReadMode::Secure ? u"secure"_s : u"fast"_s));

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
        std::fputs(u"\r\033[KTrack %1 of %2, %3 %"_s.arg(selected.indexOf(trackNumber) + 1)
                       .arg(selected.size())
                       .arg(int(done * 100 / total))
                       .toLocal8Bit()
                       .constData(),
                   stderr);
        std::fflush(stderr);
        progressShown = true;
    });
    QObject::connect(&job, &RipJob::cdgProgress, &loop, [&](int pass, qint64 done, qint64 total, int, qint64) {
        if (!progressLine || total <= 0)
            return;
        std::fputs(u"\r\033[KCD+G graphics, pass %1, %2 %"_s.arg(pass).arg(int(done * 100 / total)).toLocal8Bit().constData(), stderr);
        std::fflush(stderr);
        progressShown = true;
    });
    QObject::connect(&job, &RipJob::message, &loop, [&](int level, const QString &text) {
        if (level >= int(Rip::LogLevel::Warning)) {
            clearProgress();
            print(stderr, (level == int(Rip::LogLevel::Error) ? u"Error: "_s : u"Warning: "_s) + text);
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
        print(stderr, u"Canceled. Finished tracks are kept; the log tells what was done."_s);
        return ExitCanceled;
    }
    if (!summary.completed) {
        print(stderr, u"The rip failed: %1"_s.arg(summary.error));
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
        print(stderr, u"Warning: cannot write %1: %2"_s.arg(cuePath, cue.errorString()));

    // --- result ---
    const int checkable = int(std::count_if(selected.cbegin(), selected.cend(), [](int n) {
        return n != 0; // AccurateRip does not cover the hidden track
    }));
    QStringList out;
    out << u"Tracks:       %1 written to %2"_s.arg(summary.files.size()).arg(QDir::toNativeSeparators(dir.absolutePath()));
    out << u"Cue sheet:    %1%2"_s.arg(QDir::toNativeSeparators(cuePath),
                                       summary.subchannel.supported ? QString() : u" (no pregaps: the drive gave no Q sub-channel)"_s);
    if (parser.isSet(cdg))
        out << u"CD+G:         %1"_s.arg(summary.cdgFiles.isEmpty() ? u"no graphics on the disc, no .cdg files"_s
                                                                    : u"%1 .cdg file(s) next to the tracks"_s.arg(summary.cdgFiles.size()));
    out << u"Log:          %1"_s.arg(QDir::toNativeSeparators(summary.logFile.isEmpty() ? u"not written"_s : summary.logFile));
    out << u"Reading:      %1"_s.arg(summary.suspiciousPositions > 0 ? u"%1 suspicious position(s), see the log"_s.arg(summary.suspiciousPositions)
                                                                     : u"no errors"_s);
    int code = ExitOk;
    if (!accurateRip) {
        out << u"AccurateRip:  NOT CHECKED (--no-accuraterip)"_s;
        code = ExitUnconfirmed;
    } else if (!summary.accurateRipError.isEmpty()) {
        out << u"AccurateRip:  not checked, the lookup failed (%1)"_s.arg(summary.accurateRipError);
        code = ExitUnconfirmed;
    } else if (summary.accurateRipMismatches > 0) {
        out << u"AccurateRip:  %1 track(s) NOT accurate"_s.arg(summary.accurateRipMismatches);
        code = ExitFailed;
    } else if (summary.accurateRipConfirmed >= checkable && !selected.contains(0)) {
        out << u"AccurateRip:  all %1 track(s) accurately ripped"_s.arg(checkable);
    } else if (summary.accurateRipConfirmed >= checkable) {
        out << u"AccurateRip:  all %1 track(s) accurately ripped; the hidden track is not in the database (it never is)"_s.arg(checkable);
        code = ExitUnconfirmed;
    } else {
        out << u"AccurateRip:  %1 of %2 track(s) confirmed, the others are not in the database"_s.arg(summary.accurateRipConfirmed).arg(checkable);
        code = ExitUnconfirmed;
    }
    if (summary.suspiciousPositions > 0)
        code = ExitFailed;
    if (!cueWritten || summary.logFile.isEmpty())
        code = code == ExitFailed ? ExitFailed : ExitError;
    print(stdout, out.join(u'\n'));
    if (!accurateRip)
        print(stderr, u"WARNING: "_s + AccurateRipOffWarning + u" \"audex-cli verify %1\" can check it later."_s.arg(QDir::toNativeSeparators(logPath)));
    return code;
}

}
