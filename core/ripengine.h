/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "core/checksums.h"
#include "core/readplan.h"
#include "core/sectorreader.h"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>

#include <functional>
#include <utility>

namespace Audex::Rip
{

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

enum class ReadMode {
    Fast, // read once, retry only on reported read errors
    Secure, // read twice with cache defeat, majority vote on mismatches
};

struct RipOptions {
    ReadMode mode = ReadMode::Secure;

    int readOffset = 0; // samples, AccurateRip convention
    bool overread = false; // try to read into lead-in / lead-out

    int burstSectors = 0; // sectors per READ CD command, 0 = reader maximum

    // How often a sector the drive does not deliver at all is read again. In
    // fast mode these are the retries after a read error; in secure mode a
    // sector that never returned data is given up after this many reads
    // instead of being re-read for all rounds. Sectors that do return data
    // are not affected: those are decided by the rounds below.
    int readErrorRetries = 5;

    // A drive that stopped answering lets every command wait for its timeout
    // (often until it is switched off); retrying would take hours. The rip is
    // aborted after this many read commands in a row timed out without data.
    int maxTimeoutsInRow = 3;

    // secure mode
    int secureWindow = 512; // sectors compared per pass
    int readsPerRound = 16;
    int requiredMatches = 8;
    int maxRounds = 5;

    // Use C2 error pointers: a copy is only trusted if no C2 errors are
    // reported. In secure mode this replaces the second pass.
    bool useC2 = false;

    // Force the drive to really re-read instead of serving its cache: before
    // every re-read a sector far away is read. The distance MUST be larger
    // than the drive's audio cache including read-ahead, otherwise the far
    // read is itself answered from the cache and flushes nothing
    // (see cache-probe).
    bool cacheDefeat = true;
    int cacheDefeatDistance = 5000; // sectors
    int cacheDefeatSectors = 1; // sectors read at the far position
    // Far reads at different positions per defeat. A drive with a segmented
    // cache keeps data until all segments were reused (calibrateCacheDefeat()).
    int cacheDefeatReads = 1;

    int readSpeed = 0; // x, 0 = maximum
    int errorReadSpeed = 0; // x while recovering errors, 0 = keep readSpeed

    // Additional sample shifts at which AccurateRip checksums are computed,
    // e.g. the offset of another pressing. Each run is read with a margin of
    // a few sectors so that the shifted windows are complete.
    QList<int> accurateRipShifts;

    // CUETools database: keep the PCM of this many samples around the start
    // and the end of each CRC window, so that the CRCs of other pressings can
    // be derived afterwards (slidingCrc32()). 0 = off.
    int ctdbShiftRange = 0;

    // Secure mode: read every segment once and keep it if the verify callback
    // accepts its checksums; otherwise discard it and extract it again securely.
    // Segments without AccurateRip checksums (Segment::accurateRip false, the
    // hidden track) cannot be confirmed and are extracted securely right away.
    bool verifyFirst = false;
};

// One output unit, typically a track. Segments that are contiguous on disc are
// extracted as one continuous stream and split afterwards.
struct Segment {
    int trackNumber = 0; // 0 = hidden track one audio
    int firstLba = 0;
    int lastLba = -1; // inclusive
    bool accurateRip = true; // compute AccurateRip checksums
    bool accurateRipFirst = false; // first audio track of the disc
    bool accurateRipLast = false; // last audio track of the disc
    int ctdbSkipFirst = 0; // samples outside the CUETools database CRC window
    int ctdbSkipLast = 0;

    // AccurateRip and the CTDB check a track as the TOC defines it: from its
    // index 01 to the next track's index 01. A file that holds the track's
    // pre-gap instead of the next one's (pre-gaps with their own track)
    // differs from that: firstLba/lastLba are the file, these the range of
    // the checksums; -1 = the same as the file. The copy CRCs are the file's.
    int checksumFirstLba = -1;
    int checksumLastLba = -1;

    qint64 byteCount() const
    {
        return qint64(lastLba - firstLba + 1) * Cdda::SectorBytes;
    }
    int checksumFirst() const
    {
        return checksumFirstLba >= 0 ? checksumFirstLba : firstLba;
    }
    int checksumLast() const
    {
        return checksumLastLba >= 0 ? checksumLastLba : lastLba;
    }
    qint64 checksumByteCount() const
    {
        return qint64(checksumLast() - checksumFirst() + 1) * Cdda::SectorBytes;
    }
};

struct SuspiciousPosition {
    int lba = 0; // disc position (offset corrected)
    bool zeroFilled = false; // no usable copy at all
    QString reason;
};

struct ShiftedChecksums {
    int shift = 0;
    quint32 v1 = 0;
    quint32 v2 = 0;
    quint32 ctdb = 0;
};

struct SegmentResult {
    Segment segment;
    quint32 crc32 = 0;
    quint32 crc32NoNull = 0;
    quint32 accurateRipV1 = 0;
    quint32 accurateRipV2 = 0;
    quint32 accurateRipFrame450 = 0;
    quint32 ctdbCrc = 0; // CRC-32 of the CTDB window (accurateRip segments)
    QList<ShiftedChecksums> accurateRipShifted; // order of RipOptions::accurateRipShifts
    QByteArray ctdbHead; // RipOptions::ctdbShiftRange: PCM around the start of the CTDB window
    QByteArray ctdbTail; // and around its end
    bool acceptedAfterOnePass = false; // verifyFirst: the single read was confirmed
    double peakPercent = 0.0;
    qint64 bytesWritten = 0;
    int rereadSectors = 0; // sectors that needed error recovery
    int paddedSectors = 0; // unreadable lead-in/lead-out area replaced by silence
    QList<SuspiciousPosition> suspicious;
};

struct RipStatistics {
    qint64 readCommands = 0;
    qint64 sectorsRequested = 0;
    qint64 cacheDefeats = 0;
    qint64 passMismatches = 0; // sectors where pass A and B differed
    qint64 recoveredByDrive = 0; // RECOVERED ERROR reports
    qint64 elapsedMs = 0;
};

struct RipResult {
    bool completed = false;
    bool canceled = false;
    bool driveStalled = false; // aborted: the drive stopped answering
    QString error;
    QList<SegmentResult> segments; // same order as passed to run()
    RipStatistics statistics;

    bool hasSuspiciousPositions() const
    {
        for (const SegmentResult &s : segments)
            if (!s.suspicious.isEmpty())
                return true;
        return false;
    }
};

// What happened to a segment, for progress displays
enum class SegmentStatus {
    Confirmed, // the checksums prove the read correct (verify callback)
    Unconfirmed, // verifyFirst: the single read was discarded, a secure extraction follows
    Rereading, // verifyFirst: the secure extraction of an unconfirmed segment starts
    Done, // extracted, not confirmed by the verify callback (or none set)
    Suspicious, // extracted, with suspicious positions
};

struct RipCallbacks {
    // Corrected PCM for a segment (index into the list given to run()).
    // Return false to abort (e.g. disk full).
    std::function<bool(int segmentIndex, QByteArrayView pcm)> write;
    // doneSectors/totalSectors: the whole rip, re-reads included (the total
    // grows when a segment has to be extracted again); segmentSectors: sectors
    // of segmentIndex delivered by the running extraction of that segment
    std::function<void(qint64 doneSectors, qint64 totalSectors, int segmentIndex, qint64 segmentSectors)> progress;
    std::function<void(int segmentIndex, SegmentStatus status)> segmentStatus;
    std::function<void(LogLevel level, const QString &message)> log;
    std::function<bool()> isCanceled;

    // verifyFirst: true if the checksums prove the single read of a segment correct
    std::function<bool(int segmentIndex, const SegmentResult &result)> verify;
    // verifyFirst: forget what write() received for a segment; false if impossible
    std::function<bool(int segmentIndex)> discard;
};

// Readable audio area without overreading: [firstLba, endLba)
struct ReadableArea {
    int firstLba = 0;
    int endLba = 0;

    bool contains(int lba) const
    {
        return lba >= firstLba && lba < endLba;
    }
};

// Position of the index-th of `reads` far reads that defeat the cache for
// [lba, lba + count). The positions are distinct, keep `distance` to the range
// and alternate between behind and in front of it. fits is false if they had
// to be placed closer than `distance` to each other.
int cacheDefeatTarget(ReadableArea area, int lba, int count, int index, int reads, int distance, int sectors, bool *fits = nullptr);

class RipEngine
{
public:
    RipEngine(SectorReader &reader, ReadableArea area, RipOptions options, RipCallbacks callbacks);

    // Synchronous; run it in a worker thread for a GUI.
    RipResult run(const QList<Segment> &segments);

private:
    class Stream;

    // One read pass over a range of drive sectors
    struct Copy {
        int lba = 0;
        QByteArray audio;
        QList<SectorStatus> status;
        QList<bool> padded; // silence inserted for an unreadable lead-in/lead-out sector

        QByteArrayView sector(int i) const
        {
            return QByteArrayView(audio).sliced(qsizetype(i) * Cdda::SectorBytes, Cdda::SectorBytes);
        }
    };

    struct SegmentState {
        Crc32 crc;
        Crc32NoNull crcNoNull;
        AccurateRipChecksum accurateRip;
        PeakMeter peak;
        ClippedCrc32 ctdb;
        QList<AccurateRipChecksum> shifted; // RipOptions::accurateRipShifts
        QList<ClippedCrc32> ctdbShifted;
    };

    enum class SectorKind {
        Readable,
        Overread,
        Padding
    };

    bool extractRun(const QList<int> &segmentIndices, ReadMode mode);
    int shiftMarginSectors() const;
    // disc range a run reads: its segments, checksum ranges reaching beyond
    // them and the margin of the shifted checksums
    std::pair<int, int> runRange(const QList<int> &segmentIndices) const;
    void scheduleRetry(int index); // verifyFirst: extract again securely after the first pass
    void resetSegment(int index);
    void updateChecksums(int index);
    void segmentComplete(int index);
    bool extractFast(const ReadPlan &plan, Stream &stream);
    bool extractSecure(const ReadPlan &plan, Stream &stream);

    SectorKind kindOf(int lba) const;
    int homogeneousLength(int lba, int maxCount) const;

    // Honours padding, splits into bursts. `needed` (size `count`, optional)
    // leaves sectors out of the read; they come back as "no data".
    Copy acquire(int lba, int count, const QList<bool> *needed = nullptr);
    SectorReadResult readRaw(int lba, int count);
    void dropOverread(Copy &copy, int index, const QString &reason);
    void defeatCache(int lba, int count);
    void recoverFast(int lba, Copy &copy, int index);
    void resolveSecure(int lba, int count, Copy &result, int offset);
    bool emitCopy(const Copy &copy, Stream &stream);

    bool trustworthy(const SectorStatus &status) const;
    QList<int> segmentsForDriveSector(int driveSector) const;
    void markSuspicious(int driveSector, bool zeroFilled, const QString &reason);
    void noteReread(int driveSector);
    void setErrorSpeed(bool recovering);

    bool canceled() const;
    void log(LogLevel level, const QString &message) const;
    void reportProgress(int segmentIndex);
    void reportStatus(int segmentIndex, SegmentStatus status);

    SectorReader &m_reader;
    ReadableArea m_area;
    RipOptions m_options;
    RipCallbacks m_callbacks;

    int m_burst = 1;
    bool m_c2 = false;
    bool m_overreadWarningShown = false;
    bool m_leadInUnreadable = false;
    bool m_leadOutUnreadable = false;
    bool m_shortDiscWarningShown = false;
    bool m_recoverySpeedActive = false;
    bool m_verifying = false; // verifyFirst: first pass running

    QList<Segment> m_segments;
    QList<SegmentState> m_segmentStates;
    QList<int> m_currentRun;
    QList<int> m_retry; // verifyFirst: segments to extract again securely
    // verifyFirst: segments whose file holds audio another segment's
    // checksums did not confirm (a pre-gap); they are extracted again as well
    QSet<int> m_forceRetry;
    RipResult m_result;
    qint64 m_totalSectors = 0;
    qint64 m_doneSectors = 0;
    bool m_aborted = false;
    int m_timeoutsInRow = 0; // read commands that timed out without data
};

// Pre-gaps with their own track (EAC: "append gaps to next track"): the file
// of a track starts at its index 00 and ends before the next track's index
// 00; the checksums keep the range of the TOC (Segment::checksumFirstLba).
// index00: track -> first sector of its pre-gap, from the Q sub-channel.
// The first audio track keeps its start (what lies before it is hidden
// track one audio). Segments must be sorted by track.
void keepPregapsWithTrack(QList<Segment> &segments, const QMap<int, int> &index00, int firstAudioTrack);
}
