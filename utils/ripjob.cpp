/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ripjob.h"

#include "core/cacheprobe.h"
#include "core/cdg.h"
#include "core/hdcd.h"
#include "core/ripreport.h"
#include "encoding/encodedoutputs.h"
#include "encoding/tagwriter.h"
#include "online/accuraterip.h"
#include "online/ctdb.h"
#include "utils/encodercommand.h"
#include "utils/imagerepair.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryFile>

#include <map>
#include <memory>
#include <vector>

using namespace Qt::StringLiterals;

namespace Audex
{

using Metadata::Field;

namespace
{

QString tr(const char *text)
{
    return QCoreApplication::translate("Audex::RipJob", text);
}

QString sanitizeFileName(QString name)
{
    static const QString forbidden = u"/\\:*?\"<>|"_s;
    for (QChar &c : name) {
        if (c.unicode() < 0x20 || forbidden.contains(c))
            c = u'_';
    }
    name = name.simplified();
    while (name.startsWith(u'.'))
        name.remove(0, 1);
    if (name.size() > 200)
        name.truncate(200);
    return name.trimmed();
}

}

QString trackFileName(const CDInfo &disc, int trackNumber, const QString &pattern, const QString &suffix, bool underscores, bool twoDigitNumber)
{
    const Metadata::Album &album = disc.metadata();
    const Metadata::Track &track = album.track(trackNumber);

    QString artist = track.text(Field::Artist);
    if (artist.isEmpty())
        artist = album.text(Field::Artist);
    if (artist.isEmpty())
        artist = tr("Unknown Artist");
    QString title = track.text(Field::Title);
    if (title.isEmpty())
        title = trackNumber == 0 ? tr("Hidden Track") : tr("Track %1").arg(trackNumber);

    QString name = pattern;
    name.replace(u"{number}"_s,
                 twoDigitNumber ? u"%1"_s.arg(disc.displayTrackNumber(trackNumber), 2, 10, QLatin1Char('0'))
                                : QString::number(disc.displayTrackNumber(trackNumber)));
    name.replace(u"{artist}"_s, artist);
    name.replace(u"{title}"_s, title);
    name.replace(u"{album}"_s, album.text(Field::Album));
    name.replace(u"{year}"_s, album.text(Field::Year));
    name = sanitizeFileName(name);
    if (underscores)
        name.replace(u' ', u'_');
    if (name.isEmpty())
        name = u"track%1"_s.arg(trackNumber, 2, 10, QLatin1Char('0'));
    return name + u'.' + suffix;
}

QString imageFileName(const CDInfo &disc, const QString &pattern, const QString &suffix, bool underscores)
{
    const Metadata::Album &album = disc.metadata();

    QString artist = album.text(Field::Artist);
    if (artist.isEmpty())
        artist = tr("Unknown Artist");
    QString title = album.text(Field::Album);
    if (title.isEmpty())
        title = tr("Unknown Album");

    QString name = pattern;
    name.replace(u"{number}"_s, u"1"_s);
    name.replace(u"{artist}"_s, artist);
    name.replace(u"{title}"_s, title);
    name.replace(u"{album}"_s, title);
    name.replace(u"{year}"_s, album.text(Field::Year));
    name = sanitizeFileName(name);
    if (underscores)
        name.replace(u' ', u'_');
    if (name.isEmpty())
        name = u"disc"_s;
    return name + u'.' + suffix;
}

RipJob::RipJob(RipRequest request, QObject *parent)
    : QObject(parent)
    , m_request(std::move(request))
{
    qRegisterMetaType<Audex::RipSummary>();
}

RipJob::~RipJob()
{
    cancel();
    wait();
}

void RipJob::start()
{
    if (isRunning())
        return;
    m_cancel = false;
    m_thread = QThread::create([this] {
        run();
    });
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);
    m_thread->start();
}

void RipJob::cancel()
{
    m_cancel = true;
    std::lock_guard lock(m_outputsMutex);
    if (m_outputs)
        m_outputs->cancel();
}

bool RipJob::isRunning() const
{
    return m_thread && m_thread->isRunning();
}

bool RipJob::wait(int timeoutMs)
{
    if (!m_thread)
        return true;
    return m_thread->wait(timeoutMs < 0 ? QDeadlineTimer(QDeadlineTimer::Forever) : QDeadlineTimer(timeoutMs));
}

void RipJob::run()
{
    RipSummary summary;
    const RipRequest &rq = m_request;
    const CDInfo &disc = rq.disc;
    const Cdda::Toc &toc = disc.toc();

    auto say = [this](Rip::LogLevel level, const QString &text) {
        Q_EMIT message(int(level), text);
    };
    auto fail = [&](const QString &error) {
        summary.error = error;
        say(Rip::LogLevel::Error, error);
        Q_EMIT finished(summary);
    };

    // --- output format ---
    const Encoding::EncoderFactory *encoder = rq.encoders ? rq.encoders->factory(rq.encoderId) : nullptr;
    if (!encoder)
        return fail(tr("The output format \"%1\" is not available.").arg(rq.encoderId));
    const QString suffix = encoder->fileSuffix(rq.encoderSettings);

    // --- segments and files ---
    QList<int> tracks = rq.tracks;
    std::sort(tracks.begin(), tracks.end());
    QList<Rip::Segment> segments;
    QStringList fileNames;
    QList<Encoding::OutputTarget> targets;
    const int firstAudio = toc.firstAudioTrackNumber();
    const int lastAudio = toc.lastAudioTrackNumber();
    const QString imageFN = rq.imageFile ? imageFileName(disc, rq.imageFileNamePattern, suffix, rq.underscores) : QString();
    const QList<int> emphasis = disc.preEmphasisTracks();
    const bool imageEmphasis = !emphasis.isEmpty() && emphasis.size() == disc.audioTrackNumbers().size();
    for (int number : std::as_const(tracks)) {
        const auto entry = disc.entry(number);
        if (!entry || !entry->audio) {
            say(Rip::LogLevel::Warning, tr("Track %1 is not an audio track and is skipped.").arg(number));
            continue;
        }
        Rip::Segment s;
        s.trackNumber = number;
        s.firstLba = entry->firstLba;
        s.lastLba = entry->lastLba;
        // checksums are per segment, also when the segments share an image file
        s.accurateRip = number != 0;
        s.accurateRipFirst = number == firstAudio;
        s.accurateRipLast = number == lastAudio;
        s.ctdbSkipFirst = s.accurateRipFirst ? Ctdb::StrideSamples : 0;
        s.ctdbSkipLast = s.accurateRipLast ? Ctdb::skipLast(toc) : 0;
        segments.append(s);
        QString name = rq.filePaths.value(number);
        if (name.isEmpty())
            name = rq.imageFile ? imageFN : trackFileName(disc, number, rq.fileNamePattern, suffix, rq.underscores, rq.twoDigitTrackNumbers);
        fileNames.append(name); // one entry per segment (ReportContext::fileNames)

        Encoding::OutputTarget target;
        target.path = QDir(rq.outputDirectory).filePath(name);
        target.frames = qint64(entry->sectorCount()) * Cdda::SamplesPerSector;
        target.tags.album = disc.metadata();
        if (rq.imageFile) {
            target.tags.trackNumber = -1; // whole disc image
            target.tags.displayTrackNumber = 1;
            target.tags.trackTotal = 1;
        } else {
            target.tags.trackNumber = number;
            target.tags.displayTrackNumber = disc.displayTrackNumber(number);
            target.tags.trackTotal = int(disc.audioTrackNumbers().size());
        }
        target.tags.embedCover = rq.embedCover;
        target.tags.coverMaxSize = rq.coverMaxSize;
        if (!rq.preEmphasisTag.isEmpty() && (rq.imageFile ? imageEmphasis : emphasis.contains(number)))
            target.tags.extra.insert(rq.preEmphasisTag, u"1"_s);
        targets.append(target);
    }
    if (segments.isEmpty())
        return fail(tr("No audio tracks selected."));
    if (!rq.imageFile) {
        QSet<QString> paths;
        for (const Encoding::OutputTarget &target : std::as_const(targets)) {
            if (paths.contains(target.path))
                return fail(tr("Several tracks would be written to %1. Use a file name scheme that contains the track number.").arg(target.path));
            paths.insert(target.path);
        }
    }
    if (!QDir().mkpath(rq.outputDirectory))
        return fail(tr("Cannot create the output folder %1.").arg(rq.outputDirectory));

    const Metadata::CoverArt &cover = disc.metadata().cover();
    if (!cover.isNull())
        say(Rip::LogLevel::Info, tr("Cover: %1").arg(cover.origin.isEmpty() ? cover.source : cover.origin));

    // --- drive ---
    say(Rip::LogLevel::Info, tr("Opening %1...").arg(rq.drive.displayName));
    OpenedReader opened = openReader(rq.drive);
    if (!opened.reader)
        return fail(tr("Cannot open the drive: %1").arg(opened.error));

    // Audex does not realign reads that are off after a seek
    if (rq.inaccurateStream)
        say(Rip::LogLevel::Warning,
            rq.inaccurateStreamMeasured
                ? tr("This drive does not read accurately: the drive test measured reads up to %1 samples off after a seek (no \"accurate stream\"). "
                     "Audex cannot compensate for this: secure mode reports almost every sector as suspicious, fast mode delivers shifted audio.")
                      .arg(rq.jitterSamples)
                : tr("The drive reports that it does not read accurately (no \"accurate stream\"; not tested). If so, secure mode reports almost "
                     "every sector as suspicious and fast mode delivers shifted audio. The drive test measures it."));

    // --- Q sub-channel: pregaps, indexes, ISRC, MCN ---
    QStringList subchannelLines;
    if (rq.subchannelSkipped)
        say(Rip::LogLevel::Info, tr("Pregaps, indexes and ISRCs are not read: the drive test found no usable Q sub-channel on this drive."));
    if (rq.scanSubchannel && !m_cancel) {
        say(Rip::LogLevel::Info,
            rq.scanIsrcMcn ? tr("Reading pregaps, indexes and ISRCs from the Q sub-channel...") : tr("Reading pregaps and indexes from the Q sub-channel..."));
        summary.subchannel = Cdda::scanSubchannel(*opened.reader, toc, rq.scanIsrcMcn, [this] {
            return m_cancel.load();
        });
        subchannelLines = Cdda::formatSubchannel(summary.subchannel, toc);
        for (Encoding::OutputTarget &target : targets) { // disc values fill gaps of the metadata
            const int number = target.tags.trackNumber;
            if (summary.subchannel.isrc.contains(number) && target.tags.album.track(number).text(Field::ISRC).isEmpty())
                target.tags.album.track(number).setText(Field::ISRC, summary.subchannel.isrc.value(number));
        }
    }

    // --- pre-gaps: at the end of the previous track or with their own ---
    QString gapHandling;
    if (!rq.imageFile) {
        if (rq.pregapsWithTrack && summary.subchannel.supported) {
            Rip::keepPregapsWithTrack(segments, summary.subchannel.index00, firstAudio);
            for (qsizetype i = 0; i < segments.size(); ++i)
                targets[i].frames = qint64(segments.at(i).lastLba - segments.at(i).firstLba + 1) * Cdda::SamplesPerSector;
            gapHandling = u"With their own track"_s;
        } else if (rq.scanSubchannel && summary.subchannel.supported) {
            gapHandling = u"Appended to previous track"_s;
        } else {
            gapHandling = u"Not detected, thus appended to previous track"_s;
            if (rq.pregapsWithTrack)
                say(Rip::LogLevel::Info, tr("The pregaps stay at the end of the previous track: they could not be read from the Q sub-channel."));
        }
    }

    // --- extraction ---
    Encoding::EncodedOutputs files(*encoder, rq.encoderSettings, targets, rq.writeTags);
    {
        std::lock_guard lock(m_outputsMutex);
        m_outputs = &files;
    }
    QElapsedTimer throttle;
    throttle.start();

    // Image, secure mode: a track the databases do not confirm after the fast
    // first pass stays in the image for now; its secure extraction goes into a
    // file of its own and replaces it in the image after the rip.
    std::map<int, std::unique_ptr<QTemporaryFile>> rereads; // segment -> raw PCM

    Rip::RipCallbacks cb;
    cb.write = [&](int segment, QByteArrayView pcm) {
        const auto it = rereads.find(segment);
        if (it != rereads.end())
            return it->second->write(pcm.data(), pcm.size()) == pcm.size();
        return files.write(segment, pcm);
    };
    cb.progress = [&](qint64 done, qint64 total, int segment, qint64 segmentSectors) {
        if (throttle.elapsed() < 100 && done < total)
            return;
        throttle.restart();
        Q_EMIT progress(done, total, segment >= 0 ? segments.at(segment).trackNumber : -1, segmentSectors);
    };
    cb.segmentStatus = [&](int segment, Rip::SegmentStatus status) {
        Q_EMIT trackStatus(segments.at(segment).trackNumber, int(status));
    };
    cb.log = [&](Rip::LogLevel level, const QString &text) {
        if (level != Rip::LogLevel::Debug)
            say(level, text);
    };
    cb.isCanceled = [this] {
        return m_cancel.load();
    };

    const Rip::ReadableArea area{toc.audioStartLba(), toc.audioEndLba()};
    Rip::RipOptions options = rq.options;
    if (rq.ctdbLookup)
        options.ctdbShiftRange = Ctdb::ShiftRange;
    if (options.cacheDefeat && options.cacheDefeatReads <= 0) {
        say(Rip::LogLevel::Info, tr("Measuring the drive cache..."));
        const Rip::CacheDefeatCalibration calibration =
            Rip::calibrateCacheDefeat(*opened.reader, area, options.cacheDefeatDistance, options.cacheDefeatSectors);
        options.cacheDefeatReads = calibration.reads;
        if (calibration.timedOut)
            say(Rip::LogLevel::Warning, tr("The drive did not answer while its cache was measured."));
        else if (calibration.defeated)
            summary.measuredCacheDefeatReads = calibration.reads;
        else
            say(Rip::LogLevel::Warning, tr("The drive cache could not be defeated reliably; re-reads may be served from the cache."));
    }

    // --- AccurateRip: database and pressing, before the extraction ---
    QList<AccurateRip::Response> arResponses;
    QString arError;
    if (rq.accurateRipLookup && !m_cancel) {
        say(Rip::LogLevel::Info, tr("Looking up the disc in the AccurateRip database..."));
        bool notFound = false;
        arResponses = AccurateRip::lookupDiscEntry(toc, &arError, &notFound, 30000, cb.isCanceled);
        if (!arError.isEmpty())
            say(Rip::LogLevel::Warning, tr("AccurateRip lookup failed: %1").arg(arError));
        else if (arResponses.isEmpty())
            say(Rip::LogLevel::Info, tr("Disc not found in the AccurateRip database."));
    }
    bool arPressingKnown = false;
    if (!arResponses.isEmpty()) {
        const QMap<int, QList<quint32>> probe = AccurateRip::probeFrame450(*opened.reader, toc, options.readOffset, 3, cb.isCanceled);
        const int required = std::min<int>(2, int(probe.size()));
        for (const AccurateRip::PressingOffset &p : AccurateRip::matchFrame450(arResponses, probe)) {
            if (p.tracks < required)
                break;
            arPressingKnown = true;
            if (p.shift != 0 && options.accurateRipShifts.isEmpty()) {
                options.accurateRipShifts.append(p.shift);
                say(Rip::LogLevel::Info,
                    tr("The AccurateRip database knows another pressing of this disc (offset %1 samples); it is checked as well.").arg(p.shift));
            }
        }
    }
    // --- CUETools database: looked up before the extraction as well ---
    QList<Ctdb::Entry> entries;
    QString ctdbError;
    if (rq.ctdbLookup && !m_cancel) {
        say(Rip::LogLevel::Info, tr("Looking up the disc in the CUETools database..."));
        entries = Ctdb::lookup(toc, &ctdbError, 30000, cb.isCanceled);
        if (!ctdbError.isEmpty())
            say(Rip::LogLevel::Warning, tr("CUETools database lookup failed: %1").arg(ctdbError));
    }
    const bool ctdbConfirms = Ctdb::canConfirmTracks(entries, toc);

    // --- secure mode: keep a track after one read if a database confirms it ---
    // An image needs to be read back to replace a track in it: a decoder for
    // its format, and the tracks one after the other in the file.
    bool contiguous = true;
    for (qsizetype i = 1; i < segments.size(); ++i)
        contiguous = contiguous && segments.at(i).firstLba == segments.at(i - 1).lastLba + 1;
    const bool canReplace = !rq.imageFile || (contiguous && encoder->createDecoder() != nullptr);
    if (rq.keepConfirmedTracks && options.mode == Rip::ReadMode::Secure && !canReplace)
        say(Rip::LogLevel::Info, tr("Tracks cannot be replaced in this image afterwards; every track is read securely right away."));
    if (rq.keepConfirmedTracks && options.mode == Rip::ReadMode::Secure && canReplace && (arPressingKnown || ctdbConfirms)) {
        options.verifyFirst = true;
        const QList<int> audio = toc.audioTrackNumbers();
        cb.verify = [&, audio](int, const Rip::SegmentResult &r) {
            if (!r.segment.accurateRip || !r.suspicious.isEmpty())
                return false;
            const int index = int(audio.indexOf(r.segment.trackNumber));
            int confidence = AccurateRip::matchConfidence(arResponses, index, r.accurateRipV1, r.accurateRipV2);
            for (const Rip::ShiftedChecksums &c : r.accurateRipShifted)
                confidence = std::max(confidence, AccurateRip::matchConfidence(arResponses, index, c.v1, c.v2));
            return confidence >= rq.minConfidence || Ctdb::trackConfidence(entries, r, toc) >= rq.minConfidence;
        };
        cb.discard = [&](int segment) {
            if (!rq.imageFile)
                return files.discard(segment);
            auto file = std::make_unique<QTemporaryFile>(QDir(rq.outputDirectory).filePath(u".audex-reread-XXXXXX.pcm"_s));
            if (!file->open())
                return false;
            rereads[segment] = std::move(file);
            return true;
        };
        say(Rip::LogLevel::Info, tr("Tracks that AccurateRip or the CUETools database confirm after one read are kept; the others are read again securely."));
    }

    say(Rip::LogLevel::Info,
        QCoreApplication::translate("Audex::RipJob", "Extracting %n track(s) to %1", nullptr, int(segments.size())).arg(rq.outputDirectory));
    // HDCD: every track is checked on its way to the file; a track read again
    // starts over
    std::vector<Hdcd::Detector> hdcd(rq.detectHdcd ? segments.size() : 0);
    if (rq.detectHdcd) {
        cb.write = [&hdcd, write = cb.write](int segment, QByteArrayView pcm) {
            hdcd[segment].feed(pcm);
            return write(segment, pcm);
        };
        if (cb.discard)
            cb.discard = [&hdcd, discard = cb.discard](int segment) {
                hdcd[segment] = Hdcd::Detector();
                return discard(segment);
            };
    }

    Rip::RipEngine engine(*opened.reader, area, options, cb);
    Rip::RipResult result = engine.run(segments);

    QMap<int, QStringList> trackNotes; // segment -> further lines in the rip log
    QList<int> hdcdSegments;
    for (qsizetype i = 0; i < qsizetype(hdcd.size()) && result.completed; ++i) {
        const Hdcd::Result r = hdcd[i].result();
        if (!r.detected)
            continue;
        hdcdSegments << int(i);
        const QString details = u"peak extend %1, transient filter %2"_s.arg(r.peakExtend ? u"on"_s : u"off"_s, r.transientFilter ? u"on"_s : u"off"_s);
        trackNotes[int(i)] << u"HDCD encoded (%1)"_s.arg(details);
        say(Rip::LogLevel::Info, tr("Track %1 is HDCD encoded (%2).").arg(disc.displayTrackNumber(segments.at(i).trackNumber)).arg(details));
    }
    // the tag goes into the files once they are complete (see below); an
    // image with the patch or the repair gets it from its target as well
    if (!rq.hdcdTag.isEmpty() && !hdcdSegments.isEmpty()) {
        for (int i : std::as_const(hdcdSegments))
            targets[rq.imageFile ? 0 : i].tags.extra.insert(rq.hdcdTag, u"1"_s);
    }
    // --- CD+G: the sub-channel after the audio, while the drive is open ---
    Cdda::CdgExtraction cdg;
    if (rq.cdgSkipped)
        say(Rip::LogLevel::Info, tr("CD+G graphics are not read: the drive test found no raw sub-channel on this drive."));
    bool cdgPresent = false;
    if (rq.readCdg && result.completed && !m_cancel) {
        // not checked when the disc was inserted (yet): a few seconds tell before the whole disc is read
        cdgPresent = rq.cdgDetected || Cdda::detectCdg(*opened.reader, toc, cb.isCanceled);
        if (!cdgPresent && !m_cancel)
            say(Rip::LogLevel::Info, tr("The disc carries no CD+G graphics; no .cdg files are written."));
    }
    if (cdgPresent && !m_cancel) {
        say(Rip::LogLevel::Info, tr("Reading the CD+G graphics (the sub-channel, twice)..."));
        cdg = Cdda::extractCdg(*opened.reader, toc, segments.first().firstLba, segments.last().lastLba, {}, cb.isCanceled);
        if (!cdg.error.isEmpty())
            say(Rip::LogLevel::Warning, tr("CD+G: %1").arg(cdg.error));
        if (cdg.found) {
            say(Rip::LogLevel::Info,
                tr("CD+G: %1 graphics packs, sub-channel %2 sector(s) off, %3 by the drive; %4 sector(s) read again, %5 without agreement.")
                    .arg(cdg.graphicsPacks)
                    .arg(cdg.shift)
                    .arg(cdg.deinterleavedByDrive ? tr("de-interleaved") : tr("not de-interleaved"))
                    .arg(cdg.rereadSectors)
                    .arg(cdg.unresolvedSectors));
        } else if (cdg.error.isEmpty() && !m_cancel) {
            say(Rip::LogLevel::Info, tr("The disc carries no CD+G graphics; no .cdg files are written."));
        }
    }
    if (result.driveStalled) {
        // every further command (unlocking the tray...) would wait for its
        // timeout; Audex leaves the drive alone until it is switched off and on
        markDriveStalled(rq.drive.id);
        result.error = tr("The drive stopped answering, the rip was aborted. Switch the drive off and on again (a USB drive: unplug it) before using it.");
        say(Rip::LogLevel::Error, result.error);
    } else {
        opened.release();
    }

    if (result.completed) {
        if (!files.finishAll()) {
            result.completed = false;
            result.canceled = m_cancel.load();
            result.error = files.error();
        } else if (!rereads.empty()) {
            // the tracks read again replace their first read in the image
            say(Rip::LogLevel::Info, tr("Writing the tracks read again into the image..."));
            ImagePatchRequest patch;
            patch.path = targets.first().path;
            patch.encoder = encoder;
            patch.encoderSettings = rq.encoderSettings;
            patch.tags = targets.first().tags;
            patch.writeTags = rq.writeTags;
            QStringList numbers;
            for (const auto &[segment, file] : rereads) {
                qint64 offset = 0;
                for (int i = 0; i < segment; ++i)
                    offset += targets.at(i).frames * Cdda::BytesPerSample;
                file->flush();
                patch.patches.append(ImagePatch{offset, file->fileName()});
                numbers << QString::number(disc.displayTrackNumber(segments.at(segment).trackNumber));
            }
            QString patchError;
            if (patchImage(patch, &patchError, cb.isCanceled)) {
                say(Rip::LogLevel::Info, tr("Tracks %1 were replaced in the image by their secure extraction.").arg(numbers.join(u", "_s)));
            } else {
                result.completed = false;
                result.canceled = m_cancel.load();
                result.error = tr("The tracks read again could not be written into the image (%1). It still contains their first read.").arg(patchError);
            }
        }
    } else {
        files.abortAll();
        if (!files.error().isEmpty() && !result.driveStalled)
            result.error = files.error(); // more precise than the engine's message
    }

    {
        std::lock_guard lock(m_outputsMutex);
        m_outputs = nullptr;
    }

    // HDCD tag: known only at the end of a track, so the complete files are
    // tagged again
    if (result.completed && rq.writeTags && !rq.hdcdTag.isEmpty()) {
        for (int i : std::as_const(hdcdSegments)) {
            const Encoding::OutputTarget &target = targets.at(rq.imageFile ? 0 : i);
            const QString suffix = QFileInfo(target.path).suffix();
            QString tagError;
            if (Encoding::TagWriter::supportsSuffix(suffix) && !Encoding::TagWriter::write(target.path, suffix, target.tags, &tagError))
                say(Rip::LogLevel::Warning, tr("Cannot write the HDCD tag into %1: %2").arg(QFileInfo(target.path).fileName(), tagError));
            if (rq.imageFile)
                break; // one file
        }
    }

    // .cdg files next to the audio files: one for an image, one per track
    if (result.completed && cdg.found) {
        const auto cdgPath = [](const QString &audioPath) {
            const QFileInfo info(audioPath);
            return QDir(info.path()).filePath(info.completeBaseName() + u".cdg"_s);
        };
        const int first = segments.first().firstLba;
        QStringList written;
        for (qsizetype i = 0; i < segments.size(); ++i) {
            if (rq.imageFile && i > 0)
                break; // one file for the whole image
            const int from = rq.imageFile ? first : segments.at(i).firstLba;
            const int to = rq.imageFile ? segments.last().lastLba : segments.at(i).lastLba;
            const QString path = cdgPath(targets.at(i).path);
            QSaveFile file(path);
            if (file.open(QIODevice::WriteOnly)
                && file.write(cdg.packs.mid(qsizetype(from - first) * Cdda::SubcodeBytes, qsizetype(to - from + 1) * Cdda::SubcodeBytes)) >= 0 && file.commit())
                written << path;
            else
                say(Rip::LogLevel::Warning, tr("Cannot write %1: %2").arg(path, file.errorString()));
        }
        if (!written.isEmpty())
            say(Rip::LogLevel::Info,
                written.size() == 1 ? tr("CD+G graphics saved to %1.").arg(written.first())
                                    : tr("CD+G graphics saved as %1 .cdg files next to the tracks.").arg(written.size()));
    }

    // --- AccurateRip verification ---
    const auto sayAccurateRip = [&](const QStringList &lines) {
        for (const QString &line : lines) {
            const QString trimmed = line.trimmed();
            if (trimmed.startsWith(u"Track") || trimmed.startsWith(u"Warning") || trimmed.startsWith(u"All tracks"))
                say(line.startsWith(u"Warning") ? Rip::LogLevel::Warning : Rip::LogLevel::Info, trimmed);
        }
    };
    QStringList accurateRipLines;
    if (rq.accurateRipLookup && result.completed) {
        if (!arError.isEmpty()) {
            accurateRipLines << u"AccurateRip verification"_s << QString() << u"     Lookup failed: %1"_s.arg(arError);
        } else {
            accurateRipLines = AccurateRip::formatVerification(arResponses, result.segments, toc, &summary.accurateRipMismatches);
            sayAccurateRip(accurateRipLines);
        }
    }

    // --- CUETools database verification ---
    const auto sayCtdb = [&](const QStringList &lines) {
        if (summary.ctdbMismatches > 0)
            say(Rip::LogLevel::Warning, lines.last());
        else
            say(Rip::LogLevel::Info, u"CTDB: "_s + lines.last().trimmed());
    };
    QStringList ctdbLines;
    if (rq.ctdbLookup && result.completed && !m_cancel) {
        if (!ctdbError.isEmpty()) {
            ctdbLines << u"CUETools database (CTDB) verification"_s << QString() << u"     Lookup failed: %1"_s.arg(ctdbError);
        } else {
            ctdbLines = Ctdb::formatVerification(entries, result.segments, toc, &summary.ctdbMismatches);
            sayCtdb(ctdbLines);
        }
    }

    // --- CUETools database repair: image only, when the database contradicts the rip ---
    QMap<int, quint32> crcBeforeRepair; // segment -> copy CRC, for the tracks the repair changed
    bool repairChecked = false;
    if (rq.imageFile && rq.ctdbRepair && contiguous && result.completed && !m_cancel && summary.ctdbMismatches > 0) {
        say(Rip::LogLevel::Info, tr("Repairing the image with the CUETools database..."));
        ImageRepairRequest repair;
        repair.path = targets.first().path;
        repair.encoder = encoder;
        repair.encoderSettings = rq.encoderSettings;
        repair.tags = targets.first().tags;
        repair.writeTags = rq.writeTags;
        repair.keepOriginal = rq.ctdbRepairKeepOriginal;
        repair.toc = toc;
        repair.firstLba = segments.first().firstLba;
        repair.entries = entries;
        const ImageRepairResult repaired = repairImage(
            repair,
            [&](const QString &text) {
                say(Rip::LogLevel::Info, text);
            },
            cb.isCanceled);
        ctdbLines << QString() << repaired.log;
        if (repaired.repaired) {
            summary.ctdbMismatches = 0;
            summary.ctdbCorrections = repaired.corrections;
            say(Rip::LogLevel::Info, tr("The image was repaired with the CUETools database (%1 values corrected).").arg(repaired.corrections));

            // the log shows the repaired image: its checksums, verified again
            say(Rip::LogLevel::Info, tr("Verifying the repaired image..."));
            ImageChecksumRequest check;
            check.path = targets.first().path;
            check.encoder = encoder;
            check.segments = segments;
            check.options = options;
            QString checkError;
            const std::optional<QList<Rip::SegmentResult>> checked = checksumImage(check, &checkError, cb.isCanceled);
            if (checked && checked->size() == result.segments.size()) {
                repairChecked = true;
                for (qsizetype i = 0; i < result.segments.size(); ++i) {
                    Rip::SegmentResult &s = result.segments[i];
                    const Rip::SegmentResult &c = checked->at(i);
                    if (c.crc32 != s.crc32)
                        crcBeforeRepair.insert(int(i), s.crc32);
                    // the history of the extraction (suspicious positions, re-reads) stays
                    s.crc32 = c.crc32;
                    s.crc32NoNull = c.crc32NoNull;
                    s.accurateRipV1 = c.accurateRipV1;
                    s.accurateRipV2 = c.accurateRipV2;
                    s.accurateRipFrame450 = c.accurateRipFrame450;
                    s.accurateRipShifted = c.accurateRipShifted;
                    s.ctdbCrc = c.ctdbCrc;
                    s.ctdbHead = c.ctdbHead;
                    s.ctdbTail = c.ctdbTail;
                    s.peakPercent = c.peakPercent;
                }
                if (rq.accurateRipLookup && arError.isEmpty()) {
                    accurateRipLines = AccurateRip::formatVerification(arResponses, result.segments, toc, &summary.accurateRipMismatches);
                    accurateRipLines.first() += u" (after the CTDB repair)"_s;
                    sayAccurateRip(accurateRipLines);
                }
                const QStringList again = Ctdb::formatVerification(entries, result.segments, toc, &summary.ctdbMismatches);
                ctdbLines << QString() << again.first() + u" after the repair"_s << again.mid(1);
                sayCtdb(again);
            } else {
                ctdbLines
                    << u"     The repaired image could not be read back (%1); the track CRCs and AccurateRip results above refer to the image before the repair."_s
                           .arg(checkError);
                say(Rip::LogLevel::Warning, tr("The repaired image could not be verified: %1").arg(checkError));
            }
        } else {
            say(Rip::LogLevel::Warning, tr("The image could not be repaired: %1").arg(repaired.log.last().trimmed()));
        }
    }

    // --- report ---
    Rip::ReportContext ctx;
    ctx.application = rq.application;
    ctx.drive = opened.driveName;
    ctx.device = rq.drive.id;
    ctx.medium = disc.mediumDescription();
    ctx.gapHandling = gapHandling;
    ctx.trackNotes = trackNotes;
    ctx.repaired = repairChecked;
    ctx.crcBeforeRepair = crcBeforeRepair;
    if (rq.inaccurateStream)
        ctx.driveNotes << (rq.inaccurateStreamMeasured
                               ? u"no accurate stream, reads up to %1 samples off (drive test): this extraction is not reliable"_s.arg(rq.jitterSamples)
                               : u"the drive reports no accurate stream (not tested): this extraction may not be reliable"_s);
    ctx.options = options;
    ctx.toc = toc;
    ctx.fileNames = fileNames;
    QStringList settingsText;
    for (auto it = rq.encoderSettings.cbegin(); it != rq.encoderSettings.cend(); ++it) {
        // the external encoder is configured with a list of arguments
        const QString value = it.value().typeId() == QMetaType::QStringList ? Encoding::commandToString(it.value().toStringList()) : it.value().toString();
        settingsText << u"%1=%2"_s.arg(it.key(), value);
    }
    ctx.encoder = u"%1 (%2)"_s.arg(encoder->displayName(), encoder->version());
    if (!settingsText.isEmpty())
        ctx.encoder += u", "_s + settingsText.join(u", "_s);
    ctx.outputNotes = files.warnings();
    ctx.accurateRip = accurateRipLines;
    ctx.ctdb = ctdbLines;
    ctx.subchannel = subchannelLines;
    for (const QString &w : ctx.outputNotes)
        say(Rip::LogLevel::Warning, w);
    QStringList report = Rip::formatReport(ctx, result);
    const QString heading = u"%1 – %2"_s.arg(disc.metadata().text(Field::Artist), disc.metadata().text(Field::Album));
    report.insert(1, QString());
    report.insert(2, heading);

    if (!rq.logFilePath.isEmpty()) {
        QDir().mkpath(QFileInfo(rq.logFilePath).absolutePath());
        QFile log(rq.logFilePath);
        if (log.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            log.write(report.join(u'\n').toUtf8() + '\n');
            summary.logFile = rq.logFilePath;
        } else {
            say(Rip::LogLevel::Warning, tr("Cannot write the rip log %1: %2").arg(rq.logFilePath, log.errorString()));
        }
    }

    summary.completed = result.completed;
    summary.canceled = result.canceled;
    summary.error = result.error;
    summary.tracks = int(segments.size());
    summary.files = files.writtenFiles();
    summary.report = report;
    for (const Rip::SegmentResult &s : std::as_const(result.segments))
        summary.suspiciousPositions += int(s.suspicious.size());
    Q_EMIT finished(summary);
}

}
