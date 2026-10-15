/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ripverify.h"

#include "core/ctdbparity.h"
#include "online/accuraterip.h"
#include "online/ctdb.h"
#include "utils/imagerepair.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Audex::Verify
{

namespace
{

QString tr(const char *text)
{
    return QCoreApplication::translate("Audex::Verify", text);
}

QString hex8(quint32 v)
{
    return u"%1"_s.arg(v, 8, 16, QLatin1Char('0')).toUpper();
}

}

// ---- the rip log ------------------------------------------------------------

std::optional<RipLog> parseRipLog(const QString &text, QString *error)
{
    // written by Rip::formatReport() and Cdda::Toc::describe()
    static const QRegularExpression header(u"^(.+) extraction log from "_s);
    static const QRegularExpression tocRow(u"^\\s+(\\d+)\\s+\\|\\s*[-\\d:.]+\\s*\\|\\s*[-\\d:.]+\\s*\\|\\s*(-?\\d+)\\s*\\|\\s*(-?\\d+)\\s*\\|\\s*(.*)$"_s);
    static const QRegularExpression leadOut(u"^\\s+Lead-out:\\s*(\\d+)"_s);
    static const QRegularExpression session(u"session (\\d+)"_s);
    static const QRegularExpression trackHeader(u"^Track (\\d+)\\s*$"_s);
    static const QRegularExpression fileName(u"^\\s+Filename (.*)$"_s);
    static const QRegularExpression range(u"^\\s+Range LBA (-?\\d+) - (-?\\d+)"_s);
    static const QRegularExpression copyCrc(u"^\\s+Copy CRC ([0-9A-Fa-f]{8})"_s);

    RipLog log;
    LoggedTrack *current = nullptr;
    const QStringList lines = text.split(u'\n');
    for (QString line : lines) {
        line.remove(u'\r');
        if (log.application.isEmpty()) {
            const QRegularExpressionMatch m = header.match(line);
            if (m.hasMatch())
                log.application = m.captured(1).trimmed();
        }
        if (QRegularExpressionMatch m = tocRow.match(line); m.hasMatch() && log.tracks.isEmpty()) {
            Cdda::Track t;
            t.number = m.captured(1).toInt();
            t.firstLba = m.captured(2).toInt();
            t.lastLba = m.captured(3).toInt();
            const QString type = m.captured(4);
            t.audio = type.startsWith(u"audio"_s);
            t.preEmphasis = type.contains(u"pre-emphasis"_s);
            t.fourChannel = type.contains(u"4 channels"_s);
            if (const QRegularExpressionMatch s = session.match(type); s.hasMatch())
                t.session = s.captured(1).toInt();
            log.toc.tracks.append(t);
            continue;
        }
        if (QRegularExpressionMatch m = leadOut.match(line); m.hasMatch() && log.tracks.isEmpty()) {
            log.toc.leadOutLba = m.captured(1).toInt();
            continue;
        }

        const bool hidden = line.trimmed() == u"Hidden track one audio"_s && !line.startsWith(u' ');
        const QRegularExpressionMatch th = trackHeader.match(line);
        if (hidden || th.hasMatch()) {
            LoggedTrack t;
            t.number = hidden ? 0 : th.captured(1).toInt();
            log.tracks.append(t);
            current = &log.tracks.last();
            continue;
        }
        if (!line.isEmpty() && !line.startsWith(u' '))
            current = nullptr; // the next section
        if (!current)
            continue;
        if (QRegularExpressionMatch m = fileName.match(line); m.hasMatch()) {
            current->file = m.captured(1);
        } else if (QRegularExpressionMatch m = range.match(line); m.hasMatch()) {
            current->firstLba = m.captured(1).toInt();
            current->lastLba = m.captured(2).toInt();
        } else if (QRegularExpressionMatch m = copyCrc.match(line); m.hasMatch()) {
            current->copyCrc = m.captured(1).toUInt(nullptr, 16);
        }
    }

    if (log.toc.isEmpty() || log.toc.leadOutLba <= 0) {
        *error = tr("This is not a rip log of Audex: it has no table of contents of the disc.");
        return std::nullopt;
    }
    if (log.tracks.isEmpty()) {
        *error = tr("The rip log lists no tracks.");
        return std::nullopt;
    }
    for (const LoggedTrack &t : std::as_const(log.tracks)) {
        if (t.file.isEmpty() || t.lastLba < t.firstLba) {
            *error = tr("The rip log is incomplete: track %1 has no file or no range.").arg(t.number);
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
        result.error = tr("Cannot read %1: %2").arg(rq.logPath, logFile.errorString());
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
            track.problem = tr("The file %1 was not found.").arg(QFileInfo(t.file).fileName());
        else if (!format || !format->createDecoder())
            track.problem = tr("%1 files cannot be read back; only lossless files (FLAC, WAVE, AIFF) can be checked.").arg(QFileInfo(t.file).suffix());
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
    QString arError;
    QList<Ctdb::Entry> entries;
    QString ctdbError;
    if (rq.online) {
        say(tr("Looking up the disc in the AccurateRip database..."));
        arResponses = AccurateRip::lookupDiscEntry(toc, &arError, nullptr, 30000, isCanceled);
        result.accurateRipAsked = arError.isEmpty();
        result.accurateRipKnown = !arResponses.isEmpty();
        if (!arResponses.isEmpty()) {
            DiscAudioReader probeReader(files);
            QString probeError;
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
        say(tr("Looking up the disc in the CUETools database..."));
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
            result.error = tr("canceled");
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
                result.tracks[trackOfSegment.at(i)].problem = tr("Not checked with the databases: the file next to it is missing.");
            }
            blockSegments << s;
            blockTracks << trackOfSegment.at(i);
        }

        say(block.size() == 1 ? tr("Reading %1...").arg(QFileInfo(block.first().path).fileName()) : tr("Reading %1 files...").arg(block.size()));
        QString readError;
        const std::optional<QList<Rip::SegmentResult>> read = checksumFiles(block, blockSegments, options, &readError, isCanceled);
        for (qsizetype i = 0; i < blockSegments.size(); ++i) {
            TrackResult &track = result.tracks[blockTracks.at(i)];
            if (!read) {
                track.problem = tr("The file could not be read: %1").arg(readError);
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
    l << tr("Verification of the rip log %1").arg(QDir::toNativeSeparators(QFileInfo(rq.logPath).absoluteFilePath()));
    l << tr("Checked on %1, the log was written by %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate), log->application);
    l << QString();
    for (const TrackResult &t : std::as_const(result.tracks)) {
        l << (t.number == 0 ? u"Hidden track one audio"_s : u"Track %1"_s.arg(t.number, 2, 10, QLatin1Char('0')));
        if (!t.path.isEmpty())
            l << u"     File %1"_s.arg(QDir::toNativeSeparators(t.path));
        if (t.crc && t.loggedCrc)
            l << (t.changed() ? u"     Copy CRC %1, CHANGED (the log says %2)"_s.arg(hex8(*t.crc), hex8(*t.loggedCrc))
                              : u"     Copy CRC %1, unchanged"_s.arg(hex8(*t.crc)));
        else if (t.crc)
            l << u"     Copy CRC %1 (none in the log)"_s.arg(hex8(*t.crc));
        if (!t.problem.isEmpty())
            l << u"     "_s + t.problem;
        l << QString();
    }
    if (rq.online) {
        if (!arError.isEmpty())
            l << u"AccurateRip verification"_s << QString() << u"     Lookup failed: %1"_s.arg(arError);
        else
            l << AccurateRip::formatVerification(arResponses, checked, toc, &result.accurateRipMismatches);
        l << QString();
        if (!ctdbError.isEmpty())
            l << u"CUETools database (CTDB) verification"_s << QString() << u"     Lookup failed: %1"_s.arg(ctdbError);
        else
            l << Ctdb::formatVerification(entries, checked, toc, &result.ctdbMismatches);
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
        result.error = tr("The databases could not be asked: %1").arg(arError.isEmpty() ? ctdbError : arError);
    } else {
        result.verdict = confirmed ? Verdict::Ok : Verdict::Unconfirmed;
    }
    if (result.verdict == Verdict::Error && result.error.isEmpty())
        result.error = tr("Not every track could be checked.");

    switch (result.verdict) {
    case Verdict::Ok:
        l << (rq.online ? tr("Result: every file is unchanged, and every track is confirmed by a database.") : tr("Result: every file is unchanged."));
        break;
    case Verdict::Unconfirmed:
        l << tr("Result: nothing is wrong, but not every track could be confirmed.");
        break;
    case Verdict::Failed:
        l << tr("Result: the rip is not correct (anymore).");
        break;
    case Verdict::Error:
        l << tr("Result: %1").arg(result.error);
        break;
    }
    return result;
}

}
