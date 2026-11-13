/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ripverify.h"

#include "core/ctdbparity.h"
#include "core/riplogformat.h"
#include "online/accuraterip.h"
#include "online/ctdb.h"
#include "utils/imagerepair.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <optional>

using namespace Qt::StringLiterals;

namespace Audex::Verify
{

namespace
{

struct Tr {
    Q_DECLARE_TR_FUNCTIONS(Audex::Verify)
};

QString hex8(quint32 v)
{
    return u"%1"_s.arg(v, 8, 16, QLatin1Char('0')).toUpper();
}

}

// ---- the rip log ------------------------------------------------------------

namespace
{

using namespace RipLogFormat;

// "data, session 2, pre-emphasis, 4 channels", as Toc::describe() writes it
void applyTrackNotes(Cdda::Track &t, QStringView notes)
{
    const QList<QStringView> items = notes.split(u", "_s);
    t.audio = !items.contains(u"data");
    t.preEmphasis = items.contains(u"pre-emphasis");
    t.fourChannel = items.contains(u"4 channels");
    for (const QStringView item : items) {
        if (item.startsWith(u"session "))
            t.session = item.mid(8).toInt();
    }
}

// the first blank-separated word, as a number; nullopt if it is not one
std::optional<int> leadingInt(QStringView text)
{
    const QStringView word = text.trimmed().split(u' ').value(0);
    bool ok = false;
    const int value = word.toInt(&ok);
    return ok ? std::optional(value) : std::nullopt;
}

// the text after a prefix, or nothing if the line does not start with it
std::optional<QStringView> after(QStringView line, QStringView indent, QStringView key)
{
    if (!line.startsWith(indent) || !line.mid(indent.size()).startsWith(key))
        return std::nullopt;
    return line.mid(indent.size() + key.size());
}

// a row of the TOC table: "  1  | 00:02.00 | 00:12.00 |  150  |  1049" with,
// in logs written before the table lost it, the type as a sixth column
std::optional<Cdda::Track> tocRow(QStringView line)
{
    const QList<QStringView> columns = line.split(TocColumnSeparator);
    if (columns.size() != 5 && columns.size() != 6)
        return std::nullopt;
    bool ok[3] = {false, false, false};
    Cdda::Track t;
    t.number = columns.at(0).trimmed().toInt(&ok[0]);
    t.firstLba = columns.at(3).trimmed().toInt(&ok[1]);
    t.lastLba = columns.at(4).trimmed().toInt(&ok[2]);
    if (!ok[0] || !ok[1] || !ok[2])
        return std::nullopt; // the table header
    if (columns.size() == 6)
        applyTrackNotes(t, columns.at(5).trimmed());
    return t;
}

}

std::optional<RipLog> parseRipLog(const QString &text, QString *error)
{
    RipLog log;
    LoggedTrack *current = nullptr;
    const QStringList lines = text.split(u'\n');
    for (QString line : lines) {
        line.remove(u'\r');
        if (log.application.isEmpty()) {
            if (const qsizetype at = line.indexOf(HeaderMark); at > 0)
                log.application = line.first(at).trimmed();
        }

        // the table of contents, before the first track section
        if (log.tracks.isEmpty()) {
            if (const auto t = tocRow(line)) {
                log.toc.tracks.append(*t);
                continue;
            }
            if (const auto rest = after(line, TocIndent, TrackNote)) {
                const qsizetype colon = rest->indexOf(u':');
                const auto number = colon > 0 ? leadingInt(rest->first(colon)) : std::nullopt;
                for (Cdda::Track &t : log.toc.tracks) {
                    if (number && t.number == *number)
                        applyTrackNotes(t, rest->mid(colon + 1).trimmed());
                }
                continue;
            }
            if (const auto rest = after(line, TocIndent, LeadOut)) {
                log.toc.leadOutLba = leadingInt(*rest).value_or(0);
                continue;
            }
        }

        // a track section: its heading starts the line
        const bool hidden = line == HiddenTrackHeading;
        const auto heading = line.startsWith(TrackHeading) ? leadingInt(QStringView(line).mid(TrackHeading.size())) : std::nullopt;
        if (hidden || heading) {
            LoggedTrack t;
            t.number = hidden ? 0 : *heading;
            log.tracks.append(t);
            current = &log.tracks.last();
            continue;
        }
        if (!line.isEmpty() && !line.startsWith(u' '))
            current = nullptr; // the next section
        if (!current)
            continue;
        if (const auto rest = after(line, TrackIndent, Filename)) {
            current->file = rest->toString();
        } else if (const auto rest = after(line, TrackIndent, Range)) {
            const QList<QStringView> words = rest->split(u' ');
            if (words.size() >= 3 && words.at(1) == u"-") {
                current->firstLba = words.at(0).toInt();
                current->lastLba = words.at(2).toInt();
            }
        } else if (const auto rest = after(line, TrackIndent, CopyCrc)) {
            bool ok = false;
            const uint crc = rest->first(qMin<qsizetype>(8, rest->size())).toUInt(&ok, 16);
            if (ok && rest->size() >= 8)
                current->copyCrc = crc;
        }
    }

    if (log.toc.isEmpty() || log.toc.leadOutLba <= 0) {
        *error = Tr::tr("This is not a rip log of Audex: it has no table of contents of the disc.");
        return std::nullopt;
    }
    if (log.tracks.isEmpty()) {
        *error = Tr::tr("The rip log lists no tracks.");
        return std::nullopt;
    }
    for (const LoggedTrack &t : std::as_const(log.tracks)) {
        if (t.file.isEmpty() || t.lastLba < t.firstLba) {
            *error = Tr::tr("The rip log is incomplete: track %1 has no file or no range.").arg(t.number);
            return std::nullopt;
        }
    }
    return log;
}

// ---- the check ----------------------------------------------------------------

QString verdictName(Verdict verdict)
{
    switch (verdict) {
    case Verdict::Ok:
        return u"ok"_s;
    case Verdict::Unconfirmed:
        return u"unconfirmed"_s;
    case Verdict::Failed:
        return u"failed"_s;
    case Verdict::Error:
        break;
    }
    return u"error"_s;
}

Result verifyRip(const Request &rq, const std::function<void(const QString &)> &message, const std::function<bool()> &isCanceled)
{
    Result result;
    const auto say = [&](const QString &text) {
        if (message)
            message(text);
    };

    QFile logFile(rq.logPath);
    if (!logFile.open(QIODevice::ReadOnly)) {
        result.error = Tr::tr("Cannot read %1: %2").arg(rq.logPath, logFile.errorString());
        return result;
    }
    QString parseError;
    const std::optional<RipLog> log = parseRipLog(QString::fromUtf8(logFile.readAll()), &parseError);
    if (!log) {
        result.error = parseError;
        return result;
    }
    result.application = log->application;
    const Cdda::Toc &toc = log->toc;
    const QList<int> audio = toc.audioTrackNumbers();
    const int firstAudio = toc.firstAudioTrackNumber();
    const int lastAudio = toc.lastAudioTrackNumber();
    const QDir dir(rq.directory.isEmpty() ? QFileInfo(rq.logPath).absolutePath() : rq.directory);

    // --- files and segments ---
    // the log names the files as written; moved rips are looked for by name
    // in the folder of the log (or the one asked for)
    const auto resolve = [&](const QString &file) {
        if (rq.directory.isEmpty() && QFileInfo::exists(file))
            return QFileInfo(file).absoluteFilePath();
        const QString moved = dir.filePath(QFileInfo(file).fileName());
        return QFileInfo::exists(moved) ? moved : QString();
    };

    QList<DiscAudioFile> files;
    QList<Rip::Segment> segments;
    QList<int> trackOfSegment; // index into result.tracks
    for (const LoggedTrack &t : log->tracks) {
        TrackResult track;
        track.number = t.number;
        track.loggedCrc = t.copyCrc;
        const QString path = resolve(t.file);
        const Encoding::EncoderFactory *format = rq.encoders ? rq.encoders->factory(QFileInfo(t.file).suffix().toLower()) : nullptr;
        if (path.isEmpty())
            track.problem = Tr::tr("The file %1 was not found.").arg(QFileInfo(t.file).fileName());
        else if (!format || !format->createDecoder())
            track.problem = Tr::tr("%1 files cannot be read back; only lossless files (FLAC, WAVE, AIFF) can be checked.").arg(QFileInfo(t.file).suffix());
        else
            track.path = path;
        result.tracks << track;
        if (track.path.isEmpty())
            continue;

        // one file per track, or one image for all of them
        if (!files.isEmpty() && files.last().path == path)
            files.last().lastLba = std::max(files.last().lastLba, t.lastLba);
        else
            files << DiscAudioFile{path, format, t.firstLba, t.lastLba};

        Rip::Segment s;
        s.trackNumber = t.number;
        s.firstLba = t.firstLba;
        s.lastLba = t.lastLba;
        s.accurateRip = t.number != 0;
        if (const Cdda::Track *disc = toc.track(t.number); disc && t.number != 0) {
            // pregaps with their own track: the checksums cover the TOC range
            if (disc->firstLba != s.firstLba)
                s.checksumFirstLba = disc->firstLba;
            if (disc->lastLba != s.lastLba)
                s.checksumLastLba = disc->lastLba;
        }
        s.accurateRipFirst = t.number == firstAudio;
        s.accurateRipLast = t.number == lastAudio;
        s.ctdbSkipFirst = s.accurateRipFirst ? Ctdb::StrideSamples : 0;
        s.ctdbSkipLast = s.accurateRipLast ? Ctdb::skipLast(toc) : 0;
        segments << s;
        trackOfSegment << int(result.tracks.size() - 1);
    }
    if (files.isEmpty()) {
        result.error = result.tracks.value(0).problem;
        return result;
    }

    // --- the databases, and other pressings ---
    Rip::RipOptions options;
    QList<AccurateRip::Response> arResponses;
    Text arError;
    QList<Ctdb::Entry> entries;
    Text ctdbError;
    if (rq.online) {
        say(Tr::tr("Looking up the disc in the AccurateRip database..."));
        arResponses = AccurateRip::lookupDiscEntry(toc, &arError, nullptr, 30000, isCanceled);
        result.accurateRipAsked = arError.isEmpty();
        result.accurateRipKnown = !arResponses.isEmpty();
        if (!arResponses.isEmpty()) {
            DiscAudioReader probeReader(files);
            Text probeError;
            if (probeReader.open(&probeError)) {
                const QMap<int, QList<quint32>> probe = AccurateRip::probeFrame450(probeReader, toc, 0, 3, isCanceled);
                const int required = std::min<int>(2, int(probe.size()));
                for (const AccurateRip::PressingOffset &p : AccurateRip::matchFrame450(arResponses, probe)) {
                    if (p.tracks < required)
                        break;
                    if (p.shift != 0 && options.accurateRipShifts.isEmpty())
                        options.accurateRipShifts.append(p.shift);
                }
            }
        }
        say(Tr::tr("Looking up the disc in the CUETools database..."));
        entries = Ctdb::lookup(toc, &ctdbError, 30000, isCanceled);
        result.ctdbAsked = ctdbError.isEmpty();
        result.ctdbKnown = !entries.isEmpty();
        options.ctdbShiftRange = Ctdb::ShiftRange;
    }

    // --- reading the files: each run of files without a gap at once ---
    QList<Rip::SegmentResult> checked;
    qsizetype first = 0;
    while (first < files.size()) {
        if (isCanceled && isCanceled()) {
            result.error = Tr::tr("canceled");
            return result;
        }
        qsizetype last = first;
        while (last + 1 < files.size() && files.at(last + 1).firstLba == files.at(last).lastLba + 1)
            ++last;
        const QList<DiscAudioFile> block = files.mid(first, last - first + 1);
        const int blockFirst = block.first().firstLba;
        const int blockLast = block.last().lastLba;

        QList<Rip::Segment> blockSegments;
        QList<int> blockTracks;
        for (qsizetype i = 0; i < segments.size(); ++i) {
            Rip::Segment s = segments.at(i);
            if (s.firstLba < blockFirst || s.lastLba > blockLast)
                continue;
            if (s.accurateRip && (s.checksumFirst() < blockFirst || s.checksumLast() > blockLast)) {
                // part of its checksum range lies in a file that is missing
                s.accurateRip = false;
                s.checksumFirstLba = s.checksumLastLba = -1;
                result.tracks[trackOfSegment.at(i)].problem = Tr::tr("Not checked with the databases: the file next to it is missing.");
            }
            blockSegments << s;
            blockTracks << trackOfSegment.at(i);
        }

        say(block.size() == 1 ? Tr::tr("Reading %1...").arg(QFileInfo(block.first().path).fileName())
                              : Tr::tr("Reading %n file(s)...", nullptr, int(block.size())));
        Text readError;
        const std::optional<QList<Rip::SegmentResult>> read = checksumFiles(block, blockSegments, options, &readError, isCanceled);
        for (qsizetype i = 0; i < blockSegments.size(); ++i) {
            TrackResult &track = result.tracks[blockTracks.at(i)];
            if (!read) {
                track.problem = Tr::tr("The file could not be read: %1").arg(readError.toString());
                continue;
            }
            const Rip::SegmentResult &r = read->at(i);
            track.crc = r.crc32;
            checked << r;
            if (!r.segment.accurateRip)
                continue;
            if (!arResponses.isEmpty()) {
                const int index = int(audio.indexOf(r.segment.trackNumber));
                int confidence = AccurateRip::matchConfidence(arResponses, index, r.accurateRipV1, r.accurateRipV2);
                for (const Rip::ShiftedChecksums &c : r.accurateRipShifted)
                    confidence = std::max(confidence, AccurateRip::matchConfidence(arResponses, index, c.v1, c.v2));
                track.accurateRipConfidence = confidence;
            }
            if (Ctdb::canConfirmTracks(entries, toc))
                track.ctdbConfidence = Ctdb::trackConfidence(entries, r, toc);
        }
        first = last + 1;
    }

    // --- report ---
    QStringList &l = result.report;
    l << Tr::tr("Verification of the rip log %1").arg(QDir::toNativeSeparators(QFileInfo(rq.logPath).absoluteFilePath()));
    l << Tr::tr("Checked on %1, the log was written by %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate), log->application);
    l << QString();
    for (const TrackResult &t : std::as_const(result.tracks)) {
        l << (t.number == 0 ? Tr::tr("Hidden track one audio") : Tr::tr("Track %1").arg(t.number, 2, 10, QLatin1Char('0')));
        if (!t.path.isEmpty())
            l << u"     "_s + Tr::tr("File %1").arg(QDir::toNativeSeparators(t.path));
        if (t.crc && t.loggedCrc)
            l << u"     "_s
                    + (t.changed() ? Tr::tr("Copy CRC %1, CHANGED (the log says %2)").arg(hex8(*t.crc), hex8(*t.loggedCrc))
                                   : Tr::tr("Copy CRC %1, unchanged").arg(hex8(*t.crc)));
        else if (t.crc)
            l << u"     "_s + Tr::tr("Copy CRC %1 (none in the log)").arg(hex8(*t.crc));
        if (!t.problem.isEmpty())
            l << u"     "_s + t.problem;
        l << QString();
    }
    if (rq.online) {
        if (!arError.isEmpty())
            l << Tr::tr("AccurateRip verification") << QString() << u"     "_s + Tr::tr("Lookup failed: %1").arg(arError.toString());
        else
            l << toStrings(AccurateRip::formatVerification(arResponses, checked, toc, &result.accurateRipMismatches));
        l << QString();
        if (!ctdbError.isEmpty())
            l << Tr::tr("CUETools database (CTDB) verification") << QString() << u"     "_s + Tr::tr("Lookup failed: %1").arg(ctdbError.toString());
        else
            l << toStrings(Ctdb::formatVerification(entries, checked, toc, &result.ctdbMismatches));
        l << QString();
    }

    // --- verdict ---
    bool changed = false;
    bool incomplete = false;
    bool confirmed = true;
    const bool ctdbDisc = result.ctdbKnown && result.ctdbMismatches == 0 && !Ctdb::canConfirmTracks(entries, toc);
    for (const TrackResult &t : std::as_const(result.tracks)) {
        changed = changed || t.changed();
        incomplete = incomplete || !t.crc || !t.problem.isEmpty();
        if (rq.online) {
            if (t.number != 0 && t.accurateRipConfidence < rq.minConfidence && t.ctdbConfidence < rq.minConfidence && !ctdbDisc)
                confirmed = false;
        } else if (!t.loggedCrc) {
            confirmed = false; // offline only the log can vouch for a file
        }
    }
    if (changed || result.accurateRipMismatches > 0 || result.ctdbMismatches > 0) {
        result.verdict = Verdict::Failed;
    } else if (incomplete) {
        result.verdict = Verdict::Error;
    } else if (!confirmed && (!arError.isEmpty() || !ctdbError.isEmpty())) {
        // not "unconfirmed": the databases may well know the disc
        result.verdict = Verdict::Error;
        result.error = Tr::tr("The databases could not be asked: %1").arg((arError.isEmpty() ? ctdbError : arError).toString());
    } else {
        result.verdict = confirmed ? Verdict::Ok : Verdict::Unconfirmed;
    }
    if (result.verdict == Verdict::Error && result.error.isEmpty())
        result.error = Tr::tr("Not every track could be checked.");

    switch (result.verdict) {
    case Verdict::Ok:
        l << (rq.online ? Tr::tr("Result: every file is unchanged, and every track is confirmed by a database.") : Tr::tr("Result: every file is unchanged."));
        break;
    case Verdict::Unconfirmed:
        l << Tr::tr("Result: nothing is wrong, but not every track could be confirmed.");
        break;
    case Verdict::Failed:
        l << Tr::tr("Result: the rip is not correct (anymore).");
        break;
    case Verdict::Error:
        l << Tr::tr("Result: %1").arg(result.error);
        break;
    }
    return result;
}

}
