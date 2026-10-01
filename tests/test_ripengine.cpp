/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Tests for the RipEngine, driven through the simulated CD drive. The tests
// are deterministic: defects are either scripted (Defect::pattern) or fire
// with probability 1.0, so no test depends on a particular random sequence.

#include "test_framework.h"

#include "core/checksums.h"
#include "core/hdcd.h"
#include "core/ripengine.h"
#include "sim/simulatedsectorreader.h"

#include <QMap>
#include <QSet>

#include <cstring>

using namespace Audex;
using namespace Audex::Rip;
using namespace Audex::Sim;
using Audex::Tests::Context;

namespace
{

// Captures what the engine hands to its callbacks.
struct RipCapture {
    QList<QByteArray> segments; // corrected PCM per segment
    QStringList log;
};

RipResult runRip(SimulatedSectorReader &reader, const ReadableArea &area, const RipOptions &options, const QList<Segment> &segments, RipCapture &capture)
{
    capture.segments.clear();
    capture.segments.resize(segments.size());
    capture.log.clear();

    RipCallbacks callbacks;
    callbacks.write = [&capture](int index, QByteArrayView data) {
        capture.segments[index].append(data.data(), data.size());
        return true;
    };
    callbacks.log = [&capture](LogLevel, const QString &message) {
        capture.log.append(message);
    };
    callbacks.isCanceled = [] {
        return false;
    };

    RipEngine engine(reader, area, options, std::move(callbacks));
    return engine.run(segments);
}

Segment makeSegment(int track, int first, int last)
{
    Segment s;
    s.trackNumber = track;
    s.firstLba = first;
    s.lastLba = last;
    return s;
}

// The bytes a correct extraction must produce (see expectedAudio()).
QByteArray expected(const SimulatedSectorReader &reader, const Segment &segment, const RipOptions &options, const ReadableArea &area)
{
    return expectedAudio(reader, segment.firstLba, segment.lastLba, options.readOffset, options.overread, area);
}

// expected(), but with the sector the engine cannot deliver replaced by
// silence (zero-filled unreadable sector).
QByteArray expectedWithSilentSector(const SimulatedSectorReader &reader, const Segment &segment, const RipOptions &options, const ReadableArea &area, int lba)
{
    QByteArray data = expected(reader, segment, options, area);
    std::memset(data.data() + qsizetype(lba - segment.firstLba) * Cdda::SectorBytes, 0, Cdda::SectorBytes);
    return data;
}

} // namespace

AUDEX_TEST("secure rip with offset and cache")
{
    // A clean disc, a drive with a typical positive offset and an audio cache
    // with read-ahead. The offset correction must reproduce the disc exactly,
    // and the cache defeat must force real re-reads for the second pass.
    const auto disc = SimulatedDisc::generate({7000}, 0, 1234);
    DriveModel model;
    model.readOffset = 137;
    model.cacheSectors = 64;
    model.cacheSegments = 2;
    model.maxSectorsPerRead = 27;
    model.seed = 7;
    SimulatedSectorReader reader(disc, model);

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.readOffset = model.readOffset;
    options.secureWindow = 512;
    options.burstSectors = 27;
    options.cacheDefeat = true;
    options.cacheDefeatDistance = 5000;

    const Segment segment = makeSegment(1, 0, 6999);
    const ReadableArea area{0, 7000};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK_MSG(t, result.completed, result.error);
    AUDEX_EQUAL(t, result.segments.size(), 1);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, segment, options, area));
    AUDEX_EQUAL(t, result.statistics.passMismatches, 0LL);
    AUDEX_CHECK(t, result.segments.at(0).suspicious.isEmpty());
    AUDEX_CHECK(t, result.statistics.cacheDefeats > 0);
    // proof that the simulated cache really served some of the re-reads
    AUDEX_CHECK(t, reader.statistics().cacheHits > 0);
}

AUDEX_TEST("secure mode recovers a noisy sector")
{
    // Sector 120 returns corrupted data on its first two physical reads (the
    // two passes) and pristine data afterwards: the passes mismatch, and the
    // error recovery resolves the sector by majority vote.
    const auto disc = SimulatedDisc::generate({300}, 0, 11);
    DriveModel model;
    model.seed = 99;
    model.cacheSectors = 0;
    SimulatedSectorReader reader(disc, model);
    reader.addDefect(120, Defect{DefectKind::Noisy, 1.0, 0, {true, true, false, false, false, false, false, false}});

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.secureWindow = 64;
    options.readsPerRound = 16;
    options.requiredMatches = 8;
    options.maxRounds = 5;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 299);
    const ReadableArea area{0, 300};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK_MSG(t, result.completed, result.error);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, segment, options, area));
    AUDEX_CHECK(t, result.segments.at(0).suspicious.isEmpty());
    AUDEX_EQUAL(t, result.statistics.passMismatches, 1LL);
    AUDEX_EQUAL(t, result.segments.at(0).rereadSectors, 1);
}

AUDEX_TEST("secure mode zero-fills a permanently unreadable sector")
{
    // Sector 40 never returns data. After the retries are exhausted the
    // engine emits silence for it and reports a suspicious position.
    const auto disc = SimulatedDisc::generate({80}, 0, 12);
    DriveModel model;
    model.seed = 5;
    model.cacheSectors = 0;
    SimulatedSectorReader reader(disc, model);
    reader.addDefect(40, Defect{DefectKind::Unreadable, 1.0, 0});

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.secureWindow = 32;
    options.readsPerRound = 4;
    options.requiredMatches = 2;
    options.maxRounds = 2;
    options.readErrorRetries = 2;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 79);
    const ReadableArea area{0, 80};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK(t, result.completed);
    AUDEX_EQUAL(t, capture.segments.at(0).size(), int(segment.byteCount()));
    AUDEX_CHECK(t, result.hasSuspiciousPositions());
    const auto suspicious = result.segments.at(0).suspicious;
    AUDEX_EQUAL(t, suspicious.size(), 1);
    AUDEX_EQUAL(t, suspicious.first().lba, 40);
    AUDEX_CHECK(t, suspicious.first().zeroFilled);
    // everything except the zero-filled sector matches the disc
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expectedWithSilentSector(reader, segment, options, area, 40));
}

AUDEX_TEST("secure mode cannot detect consistently wrong data")
{
    // The drive interpolates sector 40: every read returns the same wrong
    // bytes, so both passes agree. Secure reading cannot detect this; only
    // AccurateRip/CTDB verification would. This test pins that limitation.
    const auto disc = SimulatedDisc::generate({80}, 0, 13);
    DriveModel model;
    model.seed = 17;
    model.cacheSectors = 0;
    SimulatedSectorReader reader(disc, model);
    reader.addDefect(40, Defect{DefectKind::Wrong, 1.0, 0});

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.secureWindow = 32;
    options.readsPerRound = 4;
    options.requiredMatches = 2;
    options.maxRounds = 2;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 79);
    const ReadableArea area{0, 80};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK(t, result.completed);
    AUDEX_CHECK(t, result.segments.at(0).suspicious.isEmpty());
    const QByteArray exp = expected(reader, segment, options, area);
    AUDEX_EQUAL(t, capture.segments.at(0).size(), exp.size());
    // everything around the defective sector is correct ...
    AUDEX_EQUAL_DATA(t, capture.segments.at(0).left(40 * Cdda::SectorBytes), exp.left(40 * Cdda::SectorBytes));
    AUDEX_EQUAL_DATA(t, capture.segments.at(0).mid(41 * Cdda::SectorBytes), exp.mid(41 * Cdda::SectorBytes));
    // ... but the sector itself contains the drive's wrong data, undetected
    AUDEX_CHECK(t, capture.segments.at(0).mid(40 * Cdda::SectorBytes, Cdda::SectorBytes) != reader.disc().audio.mid(40 * Cdda::SectorBytes, Cdda::SectorBytes));
}

AUDEX_TEST("fast mode recovers a transient read error")
{
    // Sector 40 fails twice (the burst and the single read of the split) and
    // succeeds on the first retry: fast mode recovers it transparently.
    const auto disc = SimulatedDisc::generate({80}, 0, 14);
    DriveModel model;
    model.seed = 18;
    model.cacheSectors = 0;
    SimulatedSectorReader reader(disc, model);
    reader.addDefect(40, Defect{DefectKind::Unreadable, 1.0, 0, {true, true, false}});

    RipOptions options;
    options.mode = ReadMode::Fast;
    options.readErrorRetries = 3;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 79);
    const ReadableArea area{0, 80};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK(t, result.completed);
    AUDEX_CHECK(t, result.segments.at(0).suspicious.isEmpty());
    AUDEX_EQUAL(t, result.segments.at(0).rereadSectors, 1);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, segment, options, area));
    AUDEX_CHECK(t, capture.log.join(QString(" ")).contains(QString("recovered")));
}

AUDEX_TEST("fast mode zero-fills a permanently unreadable sector")
{
    const auto disc = SimulatedDisc::generate({80}, 0, 14);
    DriveModel model;
    model.seed = 18;
    model.cacheSectors = 0;
    SimulatedSectorReader reader(disc, model);
    reader.addDefect(40, Defect{DefectKind::Unreadable, 1.0, 0});

    RipOptions options;
    options.mode = ReadMode::Fast;
    options.readErrorRetries = 3;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 79);
    const ReadableArea area{0, 80};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK(t, result.completed);
    AUDEX_EQUAL(t, result.segments.at(0).suspicious.size(), 1);
    AUDEX_EQUAL(t, result.segments.at(0).suspicious.first().lba, 40);
    AUDEX_CHECK(t, result.segments.at(0).suspicious.first().zeroFilled);
    AUDEX_CHECK(t, result.segments.at(0).rereadSectors >= 1);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expectedWithSilentSector(reader, segment, options, area, 40));
}

AUDEX_TEST("a drive that stops answering aborts the rip after a few timeouts")
{
    // the first read with C2 pointers hangs the drive: from then on every
    // command times out (as a Lite-On slim drive in a USB case did)
    const auto disc = SimulatedDisc::generate({300, 300}, 0, 21);
    DriveModel model;
    model.c2Capable = true;
    model.c2BlockReadsTimeOut = true;
    model.stallsAfterTimeouts = 1;
    SimulatedSectorReader reader(disc, model);

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.useC2 = true;
    options.cacheDefeat = false;

    const ReadableArea area{0, 600};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {makeSegment(1, 0, 299), makeSegment(2, 300, 599)}, capture);

    AUDEX_CHECK(t, result.driveStalled);
    AUDEX_CHECK(t, !result.completed);
    AUDEX_CHECK(t, !result.canceled);
    AUDEX_CHECK_MSG(t, result.error.contains(QStringLiteral("stopped answering")), result.error);
    // three read commands without data (each: the block and its first single sector)
    AUDEX_CHECK_MSG(t, reader.statistics().timeouts <= 6, QString::number(reader.statistics().timeouts));
}

AUDEX_TEST("single timeouts between answers do not abort the rip")
{
    // every command the drive answers resets the count
    const auto disc = SimulatedDisc::generate({120}, 0, 22);
    DriveModel model;
    model.cacheSectors = 0;
    SimulatedSectorReader reader(disc, model);

    RipOptions options;
    options.mode = ReadMode::Fast;
    options.cacheDefeat = false;
    options.maxTimeoutsInRow = 1; // strictest setting: only a timeout without data counts
    reader.addDefect(60, Defect{DefectKind::Unreadable, 1.0, 0}); // a read error is an answer

    const ReadableArea area{0, 120};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {makeSegment(1, 0, 119)}, capture);
    AUDEX_CHECK(t, result.completed);
    AUDEX_CHECK(t, !result.driveStalled);
}

AUDEX_TEST("C2 pointers: a clean re-read is trusted")
{
    // Sector 50 is corrupted and C2-flagged in the first pass, pristine and
    // unflagged on the re-read: the error recovery accepts it immediately.
    const auto disc = SimulatedDisc::generate({100}, 0, 15);
    DriveModel model;
    model.seed = 19;
    model.cacheSectors = 0;
    model.c2Capable = true;
    SimulatedSectorReader reader(disc, model);
    reader.addDefect(50, Defect{DefectKind::Noisy, 1.0, 294, {true, false}});

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.useC2 = true;
    options.secureWindow = 32;
    options.readsPerRound = 4;
    options.requiredMatches = 2;
    options.maxRounds = 2;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 99);
    const ReadableArea area{0, 100};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK(t, result.completed);
    AUDEX_CHECK(t, result.segments.at(0).suspicious.isEmpty());
    AUDEX_EQUAL(t, result.segments.at(0).rereadSectors, 1);
    AUDEX_EQUAL(t, result.statistics.passMismatches, 0LL); // C2 replaces the second pass
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, segment, options, area));
}

AUDEX_TEST("C2 flagged data that stays flagged is suspicious")
{
    // Every copy of sector 50 arrives corrupted and C2-flagged: the recovery
    // gives up, keeps the best copy and reports the position.
    const auto disc = SimulatedDisc::generate({100}, 0, 15);
    DriveModel model;
    model.seed = 19;
    model.cacheSectors = 0;
    model.c2Capable = true;
    SimulatedSectorReader reader(disc, model);
    reader.addDefect(50, Defect{DefectKind::Noisy, 1.0, 1});

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.useC2 = true;
    options.secureWindow = 32;
    options.readsPerRound = 4;
    options.requiredMatches = 2;
    options.maxRounds = 2;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 99);
    const ReadableArea area{0, 100};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK(t, result.completed);
    AUDEX_CHECK(t, result.hasSuspiciousPositions());
    AUDEX_EQUAL(t, result.segments.at(0).suspicious.size(), 1);
    AUDEX_EQUAL(t, result.segments.at(0).suspicious.first().lba, 50);
    AUDEX_CHECK(t, !result.segments.at(0).suspicious.first().zeroFilled);
    AUDEX_EQUAL(t, capture.segments.at(0).size(), int(segment.byteCount()));
}

AUDEX_TEST("lead-out overread supplies the offset margin")
{
    // With a one sector offset the last disc sector lives in the lead-out
    // sector. A drive that can overread delivers it: no padding, no silence.
    const auto disc = SimulatedDisc::generate({100}, 0, 16);
    DriveModel model;
    model.readOffset = Cdda::SamplesPerSector;
    model.leadOutReadable = true;
    model.cacheSectors = 0;
    model.seed = 20;
    SimulatedSectorReader reader(disc, model);

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.readOffset = model.readOffset;
    options.overread = true;
    options.secureWindow = 32;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 99);
    const ReadableArea area{0, 100};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK_MSG(t, result.completed, result.error);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, segment, options, area));
    AUDEX_CHECK(t, result.segments.at(0).suspicious.isEmpty());
    AUDEX_EQUAL(t, result.segments.at(0).paddedSectors, 0);
}

AUDEX_TEST("lead-in overread supplies the offset margin")
{
    // The same at the start of the disc: with a negative offset the first
    // disc sector lives in the lead-in.
    const auto disc = SimulatedDisc::generate({100}, 0, 16);
    DriveModel model;
    model.readOffset = -Cdda::SamplesPerSector;
    model.leadInReadable = true;
    model.cacheSectors = 0;
    model.seed = 20;
    SimulatedSectorReader reader(disc, model);

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.readOffset = model.readOffset;
    options.overread = true;
    options.secureWindow = 32;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 99);
    const ReadableArea area{0, 100};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK_MSG(t, result.completed, result.error);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, segment, options, area));
    AUDEX_CHECK(t, result.segments.at(0).suspicious.isEmpty());
    AUDEX_EQUAL(t, result.segments.at(0).paddedSectors, 0);
}

AUDEX_TEST("HDCD: whole tracks in any chunks, and no false alarm on noise")
{
    // the rip feeds every track to the detector in the chunks the engine
    // delivers: the packets are found across chunk borders
    static const unsigned char packets[] = {
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    };
    const QByteArrayView all(reinterpret_cast<const char *>(packets), sizeof(packets));
    Audex::Hdcd::Detector chunked;
    for (qsizetype at = 0, step = 3; at < all.size(); at += step, step = step % 7 + 2)
        chunked.feed(all.sliced(at, std::min<qsizetype>(step, all.size() - at)));
    AUDEX_CHECK(t, chunked.result().detected);
    AUDEX_EQUAL(t, chunked.result().packets, 2);

    // a whole disc of noise (the LSBs random): no packets
    const auto disc = SimulatedDisc::generate({4500, 4500}, 0, 77);
    Audex::Hdcd::Detector noise;
    for (qsizetype at = 0; at < disc.audio.size(); at += 27 * Cdda::SectorBytes)
        noise.feed(QByteArrayView(disc.audio).sliced(at, std::min<qsizetype>(27 * Cdda::SectorBytes, disc.audio.size() - at)));
    AUDEX_CHECK_MSG(t, !noise.result().detected, QString::number(noise.result().packets));
}

AUDEX_TEST("missing overread capability falls back to silence")
{
    // The drive cannot read into the lead-out: the samples that would need
    // the overread sector are replaced by silence and counted as padded.
    const auto disc = SimulatedDisc::generate({100}, 0, 16);
    DriveModel model;
    model.readOffset = Cdda::SamplesPerSector;
    model.leadOutReadable = false;
    model.cacheSectors = 0;
    model.seed = 20;
    SimulatedSectorReader reader(disc, model);

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.readOffset = model.readOffset;
    options.overread = true; // requested, but the drive cannot do it
    options.secureWindow = 32;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 99);
    const ReadableArea area{0, 100};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    AUDEX_CHECK_MSG(t, result.completed, result.error);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, segment, options, area));
    AUDEX_CHECK(t, result.segments.at(0).suspicious.isEmpty());
    AUDEX_EQUAL(t, result.segments.at(0).paddedSectors, 1);
}

AUDEX_TEST("a contiguous read is split into tracks")
{
    // Two contiguous tracks are extracted as one continuous stream; the
    // stream must be split at exactly the track boundary.
    const auto disc = SimulatedDisc::generate({60, 70}, 0, 17);
    DriveModel model;
    model.cacheSectors = 0;
    model.seed = 21;
    SimulatedSectorReader reader(disc, model);

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.secureWindow = 32;
    options.cacheDefeat = false;

    const Segment first = makeSegment(1, 0, 59);
    const Segment second = makeSegment(2, 60, 129);
    const ReadableArea area{0, 130};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {first, second}, capture);

    AUDEX_CHECK(t, result.completed);
    AUDEX_EQUAL(t, capture.segments.size(), 2);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, first, options, area));
    AUDEX_EQUAL_DATA(t, capture.segments.at(1), expected(reader, second, options, area));
    AUDEX_EQUAL(t, result.segments.at(0).bytesWritten, first.byteCount());
    AUDEX_EQUAL(t, result.segments.at(1).bytesWritten, second.byteCount());
}

AUDEX_TEST("verifyFirst for an image: the secure re-read of a track replaces its first read")
{
    // The rip job keeps an unconfirmed track in the image during the first
    // pass and writes its secure extraction into a file of its own, which then
    // replaces the track in the image. The engine has to deliver the whole
    // image in the first pass and exactly the track's bytes afterwards.
    const auto disc = SimulatedDisc::generate({100, 100, 100}, 0, 23);
    DriveModel model;
    model.cacheSectors = 0;
    model.seed = 24;
    SimulatedSectorReader reader(disc, model);
    // the first read of sector 150 (track 2) is wrong, the later ones are right
    reader.addDefect(150, Defect{DefectKind::Noisy, 1.0, 0, {true, false}});

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.verifyFirst = true;
    options.secureWindow = 32;
    options.cacheDefeat = false;

    const QList<Segment> segments{makeSegment(1, 0, 99), makeSegment(2, 100, 199), makeSegment(3, 200, 299)};
    const ReadableArea area{0, 300};
    QList<quint32> crcs; // of the correct audio
    for (const Segment &s : segments) {
        Crc32 crc;
        crc.update(expected(reader, s, options, area));
        crcs << crc.value();
    }

    QByteArray image;
    QMap<int, QByteArray> rereads;
    QSet<int> discarded;
    RipCallbacks callbacks;
    callbacks.write = [&](int index, QByteArrayView data) {
        if (discarded.contains(index))
            rereads[index].append(data.data(), data.size());
        else
            image.append(data.data(), data.size());
        return true;
    };
    callbacks.verify = [&crcs](int index, const SegmentResult &r) {
        return r.crc32 == crcs.at(index);
    };
    callbacks.discard = [&discarded](int index) {
        discarded.insert(index); // stays in the image for now
        return true;
    };
    callbacks.isCanceled = [] {
        return false;
    };
    RipEngine engine(reader, area, options, std::move(callbacks));
    const RipResult result = engine.run(segments);

    AUDEX_CHECK(t, result.completed);
    AUDEX_CHECK(t, discarded == QSet<int>{1});
    AUDEX_CHECK(t, result.segments.at(0).acceptedAfterOnePass && result.segments.at(2).acceptedAfterOnePass);
    AUDEX_EQUAL(t, image.size(), qsizetype(300) * Cdda::SectorBytes); // the whole image in the first pass
    AUDEX_EQUAL(t, rereads.value(1).size(), qsizetype(100) * Cdda::SectorBytes);
    AUDEX_CHECK(t, result.segments.at(1).crc32 == crcs.at(1)); // checksums of the secure read

    image.replace(qsizetype(100) * Cdda::SectorBytes, rereads.value(1).size(), rereads.value(1));
    QByteArray correct;
    for (const Segment &s : segments)
        correct += expected(reader, s, options, area);
    AUDEX_EQUAL_DATA(t, image, correct);
}

AUDEX_TEST("verifyFirst discards an unconfirmed track and re-extracts it securely")
{
    const auto disc = SimulatedDisc::generate({100}, 0, 18);
    DriveModel model;
    model.cacheSectors = 0;
    model.seed = 22;
    SimulatedSectorReader reader(disc, model);

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.verifyFirst = true;
    options.secureWindow = 32;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 99);
    const ReadableArea area{0, 100};
    RipCapture capture;
    int verifyCalls = 0;
    bool discarded = false;

    RipCallbacks callbacks;
    capture.segments.resize(1);
    callbacks.write = [&capture](int index, QByteArrayView data) {
        capture.segments[index].append(data.data(), data.size());
        return true;
    };
    callbacks.verify = [&verifyCalls](int, const SegmentResult &) {
        ++verifyCalls;
        return false; // checksums not in the database: reject the fast read
    };
    callbacks.discard = [&capture, &discarded](int index) {
        discarded = true;
        capture.segments[index].clear();
        return true;
    };
    callbacks.isCanceled = [] {
        return false;
    };

    RipEngine engine(reader, area, options, std::move(callbacks));
    const RipResult result = engine.run({segment});

    AUDEX_CHECK(t, result.completed);
    // after the fast read, and after the secure one (for the status only)
    AUDEX_EQUAL(t, verifyCalls, 2);
    AUDEX_CHECK(t, discarded);
    AUDEX_CHECK(t, !result.segments.at(0).acceptedAfterOnePass);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, segment, options, area));
    // 4 bursts for the fast read, then the secure re-extraction
    AUDEX_EQUAL(t, reader.statistics().commands, 4LL + 14LL);
}

AUDEX_TEST("verifyFirst keeps a confirmed track")
{
    const auto disc = SimulatedDisc::generate({100}, 0, 18);
    DriveModel model;
    model.cacheSectors = 0;
    model.seed = 22;
    SimulatedSectorReader reader(disc, model);

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.verifyFirst = true;
    options.secureWindow = 32;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 99);
    const ReadableArea area{0, 100};
    RipCapture capture;
    int verifyCalls = 0;
    bool discarded = false;

    RipCallbacks callbacks;
    capture.segments.resize(1);
    callbacks.write = [&capture](int index, QByteArrayView data) {
        capture.segments[index].append(data.data(), data.size());
        return true;
    };
    callbacks.verify = [&verifyCalls](int, const SegmentResult &) {
        ++verifyCalls;
        return true; // AccurateRip confirms the single read
    };
    callbacks.discard = [&discarded](int) {
        discarded = true;
        return true;
    };
    callbacks.isCanceled = [] {
        return false;
    };

    RipEngine engine(reader, area, options, std::move(callbacks));
    const RipResult result = engine.run({segment});

    AUDEX_CHECK(t, result.completed);
    AUDEX_EQUAL(t, verifyCalls, 1);
    AUDEX_CHECK(t, !discarded);
    AUDEX_CHECK(t, result.segments.at(0).acceptedAfterOnePass);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, segment, options, area));
    // one fast read, no secure re-extraction
    AUDEX_EQUAL(t, reader.statistics().commands, 4LL);
}

AUDEX_TEST("segment status and progress follow verifyFirst and the secure re-reads")
{
    // track 1 is confirmed at once, track 2 after its secure re-read, track 3
    // contains an unreadable sector and stays suspicious
    const auto disc = SimulatedDisc::generate({180}, 0, 18);
    DriveModel model;
    model.cacheSectors = 0;
    model.seed = 23;
    SimulatedSectorReader reader(disc, model);
    reader.addDefect(150, Defect{DefectKind::Unreadable, 1.0, 0});

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.verifyFirst = true;
    options.secureWindow = 32;
    options.readsPerRound = 4;
    options.requiredMatches = 2;
    options.maxRounds = 2;
    options.readErrorRetries = 2;
    options.cacheDefeat = false;

    const QList<Segment> segments{makeSegment(1, 0, 59), makeSegment(2, 60, 119), makeSegment(3, 120, 179)};
    const ReadableArea area{0, 180};

    QList<QPair<int, SegmentStatus>> statuses;
    QList<qint64> maxSectors(3, 0); // per segment, of the last extraction
    QList<int> verifyCalls(3, 0);
    qint64 lastDone = 0;
    qint64 lastTotal = 0;
    bool monotonic = true;

    RipCallbacks callbacks;
    callbacks.write = [](int, QByteArrayView) {
        return true;
    };
    callbacks.progress = [&](qint64 done, qint64 total, int segment, qint64 segmentSectors) {
        monotonic = monotonic && done >= lastDone && done <= total;
        lastDone = done;
        lastTotal = total;
        if (segment >= 0)
            maxSectors[segment] = std::max(maxSectors.at(segment), segmentSectors);
    };
    callbacks.segmentStatus = [&](int segment, SegmentStatus status) {
        statuses.append({segment, status});
        if (status == SegmentStatus::Rereading)
            maxSectors[segment] = 0;
    };
    callbacks.verify = [&](int segment, const SegmentResult &r) {
        const int call = ++verifyCalls[segment];
        if (!r.suspicious.isEmpty())
            return false;
        return segment == 0 || (segment == 1 && call == 2);
    };
    callbacks.discard = [](int) {
        return true;
    };
    callbacks.isCanceled = [] {
        return false;
    };

    RipEngine engine(reader, area, options, std::move(callbacks));
    const RipResult result = engine.run(segments);
    AUDEX_CHECK(t, result.completed);

    const QList<QPair<int, SegmentStatus>> expectedStatuses{
        {0, SegmentStatus::Confirmed},
        {1, SegmentStatus::Unconfirmed},
        {2, SegmentStatus::Unconfirmed},
        {1, SegmentStatus::Rereading},
        {1, SegmentStatus::Confirmed},
        {2, SegmentStatus::Rereading},
        {2, SegmentStatus::Suspicious},
    };
    AUDEX_CHECK(t, statuses == expectedStatuses);
    // the progress never exceeds the total, which grows by the re-reads
    AUDEX_CHECK(t, monotonic);
    AUDEX_EQUAL(t, lastDone, lastTotal);
    AUDEX_CHECK(t, lastTotal > 180);
    // segment progress restarts with the re-read and ends at the segment size
    // (in a run of several segments the last report may already name the next)
    AUDEX_CHECK(t, maxSectors.at(0) > 0 && maxSectors.at(0) <= 60);
    AUDEX_EQUAL(t, maxSectors.at(1), qint64(60));
    AUDEX_EQUAL(t, maxSectors.at(2), qint64(60));
    AUDEX_CHECK(t, result.segments.at(0).acceptedAfterOnePass);
    AUDEX_CHECK(t, !result.segments.at(2).suspicious.isEmpty());
}

AUDEX_TEST("verifyFirst reads the hidden track securely right away")
{
    // the hidden track has no AccurateRip checksums: it can never be
    // confirmed, so it is not read fast and discarded first
    const auto disc = SimulatedDisc::generate({100}, 0, 18);
    DriveModel model;
    model.cacheSectors = 0;
    model.seed = 24;
    SimulatedSectorReader reader(disc, model);

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.verifyFirst = true;
    options.secureWindow = 32;
    options.cacheDefeat = false;

    Segment hidden = makeSegment(0, 0, 29);
    hidden.accurateRip = false;
    const Segment track = makeSegment(1, 30, 99);
    const ReadableArea area{0, 100};
    RipCapture capture;
    capture.segments.resize(2);
    QList<int> verified;
    QList<int> discarded;
    QList<QPair<int, SegmentStatus>> statuses;

    RipCallbacks callbacks;
    callbacks.write = [&capture](int index, QByteArrayView data) {
        capture.segments[index].append(data.data(), data.size());
        return true;
    };
    callbacks.verify = [&verified](int index, const SegmentResult &) {
        verified.append(index);
        return true;
    };
    callbacks.discard = [&discarded](int index) {
        discarded.append(index);
        return true;
    };
    callbacks.segmentStatus = [&statuses](int index, SegmentStatus status) {
        statuses.append({index, status});
    };
    callbacks.isCanceled = [] {
        return false;
    };

    RipEngine engine(reader, area, options, std::move(callbacks));
    const RipResult result = engine.run({hidden, track});

    AUDEX_CHECK(t, result.completed);
    AUDEX_CHECK(t, verified == QList<int>{1});
    AUDEX_CHECK(t, discarded.isEmpty());
    const QList<QPair<int, SegmentStatus>> expectedStatuses{{0, SegmentStatus::Done}, {1, SegmentStatus::Confirmed}};
    AUDEX_CHECK(t, statuses == expectedStatuses);
    AUDEX_CHECK(t, !result.segments.at(0).acceptedAfterOnePass);
    AUDEX_CHECK(t, result.segments.at(1).acceptedAfterOnePass);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, hidden, options, area));
    AUDEX_EQUAL_DATA(t, capture.segments.at(1), expected(reader, track, options, area));
    // hidden track: two passes of 30 sectors (2 windows each); track: one fast read
    AUDEX_EQUAL(t, reader.statistics().physicalSectors, 2LL * 30 + 70);
}

AUDEX_TEST("HDCD control packets are detected")
{
    static const unsigned char pcm[] = {
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    };
    Audex::Hdcd::Detector detector;
    detector.feed(QByteArrayView(reinterpret_cast<const char *>(pcm), sizeof(pcm)));
    const Audex::Hdcd::Result r = detector.result();
    AUDEX_CHECK(t, r.detected);
    AUDEX_CHECK(t, r.peakExtend);
    AUDEX_EQUAL(t, r.packets, 2);
    AUDEX_EQUAL(t, r.errors, 0);
}

AUDEX_TEST("silence is not HDCD")
{
    const QByteArray pcm(8192, '\0');
    Audex::Hdcd::Detector detector;
    detector.feed(pcm);
    AUDEX_CHECK(t, !detector.result().detected);
    AUDEX_EQUAL(t, detector.result().errors, 0);
}

AUDEX_TEST("unstable overread data falls back to silence")
{
    // The drive delivers the lead-out sector, but differently on every read
    // after the first: no error recovery, the overread side is dropped.
    const auto disc = SimulatedDisc::generate({100}, 0, 16);
    DriveModel model;
    model.readOffset = Cdda::SamplesPerSector;
    model.leadOutReadable = true;
    model.cacheSectors = 0;
    model.seed = 20;
    SimulatedSectorReader reader(disc, model);
    QList<bool> pattern(64, true);
    pattern[0] = false;
    reader.addDefect(100, Defect{DefectKind::Noisy, 1.0, 0, pattern});

    RipOptions options;
    options.mode = ReadMode::Secure;
    options.readOffset = model.readOffset;
    options.overread = true;
    options.secureWindow = 32;
    options.cacheDefeat = false;

    const Segment segment = makeSegment(1, 0, 99);
    const ReadableArea area{0, 100};
    RipCapture capture;
    const RipResult result = runRip(reader, area, options, {segment}, capture);

    RipOptions withoutOverread = options;
    withoutOverread.overread = false;
    AUDEX_CHECK_MSG(t, result.completed, result.error);
    AUDEX_EQUAL_DATA(t, capture.segments.at(0), expected(reader, segment, withoutOverread, area));
    AUDEX_CHECK(t, result.segments.at(0).suspicious.isEmpty());
    AUDEX_EQUAL(t, result.segments.at(0).paddedSectors, 1);
    AUDEX_EQUAL(t, result.segments.at(0).rereadSectors, 0);
}

namespace
{

QList<Segment> threeTracks()
{
    QList<Segment> segments{makeSegment(1, 0, 999), makeSegment(2, 1000, 1999), makeSegment(3, 2000, 2999)};
    segments[0].accurateRipFirst = true;
    segments[2].accurateRipLast = true;
    return segments;
}

// pre-gaps: 2 s before track 2, 1 s before track 3
const QMap<int, int> Index00{{2, 850}, {3, 1925}};

}

AUDEX_TEST("pregaps with their own track: the files move, the checksums stay")
{
    const auto disc = SimulatedDisc::generate({1000, 1000, 1000}, 0, 41);
    DriveModel model;
    model.seed = 5;
    RipOptions options;
    options.mode = ReadMode::Fast;
    options.accurateRipShifts = {30};
    options.ctdbShiftRange = 64;
    const ReadableArea area{0, 3000};

    SimulatedSectorReader readerA(disc, model);
    RipCapture appended;
    const RipResult a = runRip(readerA, area, options, threeTracks(), appended);

    QList<Segment> moved = threeTracks();
    keepPregapsWithTrack(moved, Index00, 1);
    AUDEX_EQUAL(t, moved.at(0).lastLba, 849);
    AUDEX_EQUAL(t, moved.at(1).firstLba, 850);
    AUDEX_EQUAL(t, moved.at(1).lastLba, 1924);
    AUDEX_EQUAL(t, moved.at(2).firstLba, 1925);
    AUDEX_EQUAL(t, moved.at(2).lastLba, 2999);
    AUDEX_EQUAL(t, moved.at(1).checksumFirst(), 1000);
    AUDEX_EQUAL(t, moved.at(1).checksumLast(), 1999);

    SimulatedSectorReader readerB(disc, model);
    RipCapture own;
    const RipResult b = runRip(readerB, area, options, moved, own);
    AUDEX_CHECK_MSG(t, a.completed && b.completed, b.error);
    for (qsizetype i = 0; i < moved.size(); ++i) {
        AUDEX_EQUAL_DATA(t, own.segments.at(i), expected(readerB, moved.at(i), options, area));
        const SegmentResult &x = a.segments.at(i);
        const SegmentResult &y = b.segments.at(i);
        AUDEX_EQUAL(t, y.accurateRipV1, x.accurateRipV1);
        AUDEX_EQUAL(t, y.accurateRipV2, x.accurateRipV2);
        AUDEX_EQUAL(t, y.accurateRipFrame450, x.accurateRipFrame450);
        AUDEX_EQUAL(t, y.ctdbCrc, x.ctdbCrc);
        AUDEX_EQUAL_DATA(t, y.ctdbHead, x.ctdbHead);
        AUDEX_EQUAL_DATA(t, y.ctdbTail, x.ctdbTail);
        AUDEX_EQUAL(t, y.accurateRipShifted.value(0).v1, x.accurateRipShifted.value(0).v1);
        AUDEX_EQUAL(t, y.accurateRipShifted.value(0).ctdb, x.accurateRipShifted.value(0).ctdb);
    }
    AUDEX_CHECK(t, b.segments.at(1).crc32 != a.segments.at(1).crc32); // the copy CRC is the file's
    AUDEX_EQUAL_DATA(t, own.segments.join(), appended.segments.join());

    // track 2 alone: its file starts with its pre-gap, which lies in track 1
    QList<Segment> single{threeTracks().at(1)};
    keepPregapsWithTrack(single, Index00, 1);
    SimulatedSectorReader readerC(disc, model);
    RipCapture alone;
    const RipResult c = runRip(readerC, area, options, single, alone);
    AUDEX_CHECK(t, c.completed);
    AUDEX_EQUAL_DATA(t, alone.segments.at(0), expected(readerC, single.at(0), options, area));
    AUDEX_EQUAL(t, c.segments.at(0).accurateRipV2, a.segments.at(1).accurateRipV2);
    AUDEX_EQUAL(t, c.segments.at(0).ctdbCrc, a.segments.at(1).ctdbCrc);

    // the first track keeps its start, a track without a known pre-gap too
    QList<Segment> partly = threeTracks();
    keepPregapsWithTrack(partly, {{1, 0}, {3, 1925}}, 1);
    AUDEX_EQUAL(t, partly.at(0).firstLba, 0);
    AUDEX_EQUAL(t, partly.at(1).firstLba, 1000);
    AUDEX_EQUAL(t, partly.at(1).lastLba, 1924);
}

AUDEX_TEST("pregaps with their own track: a file with an unconfirmed pregap is read again")
{
    // track 2 is not confirmed after the first read; its checksum range ends
    // in the file of track 3 (the pre-gap of track 3), so track 3 is read
    // again as well, track 1 is kept
    const auto disc = SimulatedDisc::generate({1000, 1000, 1000}, 0, 43);
    DriveModel model;
    model.cacheSectors = 0;
    model.seed = 9;
    SimulatedSectorReader reader(disc, model);
    RipOptions options;
    options.mode = ReadMode::Secure;
    options.verifyFirst = true;
    options.cacheDefeat = false;
    const ReadableArea area{0, 3000};
    QList<Segment> segments = threeTracks();
    keepPregapsWithTrack(segments, Index00, 1);

    RipCapture capture;
    capture.segments.resize(3);
    QList<int> discarded;
    QList<int> unconfirmed;
    RipCallbacks callbacks;
    callbacks.write = [&capture](int index, QByteArrayView data) {
        capture.segments[index].append(data.data(), data.size());
        return true;
    };
    callbacks.verify = [](int index, const SegmentResult &) {
        return index != 1;
    };
    callbacks.discard = [&](int index) {
        discarded << index;
        capture.segments[index].clear();
        return true;
    };
    callbacks.segmentStatus = [&](int index, SegmentStatus status) {
        if (status == SegmentStatus::Unconfirmed)
            unconfirmed << index;
    };
    callbacks.isCanceled = [] {
        return false;
    };
    RipEngine engine(reader, area, options, std::move(callbacks));
    const RipResult result = engine.run(segments);

    AUDEX_CHECK_MSG(t, result.completed, result.error);
    AUDEX_CHECK_MSG(t, discarded == QList<int>({1, 2}), QString::number(discarded.size()));
    AUDEX_CHECK(t, unconfirmed == QList<int>({1, 2}));
    AUDEX_CHECK(t, result.segments.at(0).acceptedAfterOnePass);
    AUDEX_CHECK(t, !result.segments.at(2).acceptedAfterOnePass);
    for (qsizetype i = 0; i < segments.size(); ++i)
        AUDEX_EQUAL_DATA(t, capture.segments.at(i), expected(reader, segments.at(i), options, area));
}
