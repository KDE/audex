/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ripengine.h"

#include <QElapsedTimer>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>

using namespace Qt::StringLiterals;

namespace Audex::Rip
{

using Cdda::SectorBytes;

// Copies the part of data (at stream position begin) that falls into buffer
// (at stream position at).
static void copyOverlap(QByteArray &buffer, qint64 at, qint64 begin, QByteArrayView data)
{
    const qint64 from = std::max(at, begin);
    const qint64 to = std::min(at + buffer.size(), begin + data.size());
    if (from < to)
        std::memcpy(buffer.data() + (from - at), data.constData() + (from - begin), to - from);
}

// ---------------------------------------------------------------------------
// Stream: turns the drive sector sequence of one run into offset corrected
// PCM and distributes it over the contiguous segments of that run.

class RipEngine::Stream
{
public:
    // leadBytes: margin read before the first segment (shifted checksums only)
    Stream(RipEngine &engine, const QList<int> &segmentIndices, const ReadPlan &plan, qint64 leadBytes)
        : m_engine(engine)
        , m_indices(segmentIndices)
        , m_skip(plan.skipBytes)
    {
        int maxShift = m_engine.m_options.ctdbShiftRange;
        for (int shift : std::as_const(m_engine.m_options.accurateRipShifts))
            maxShift = std::max(maxShift, shift);
        const qint64 completionDelay = qint64(maxShift) * Cdda::BytesPerSample;
        qint64 position = leadBytes;
        for (int index : m_indices) {
            const Segment &s = m_engine.m_segments.at(index);
            m_starts.append(position);
            m_sumStarts.append(position + qint64(s.checksumFirst() - s.firstLba) * SectorBytes);
            // complete once the file and the checksums (shifted ones included) have their data
            m_ends.append(std::max(position + s.byteCount(), m_sumStarts.last() + s.checksumByteCount()) + completionDelay);
            position += s.byteCount();
        }
        m_notified.fill(false, m_indices.size());
    }

    bool feed(QByteArrayView data)
    {
        if (m_skip > 0) {
            const qsizetype n = std::min<qsizetype>(m_skip, data.size());
            data = data.sliced(n);
            m_skip -= n;
        }
        const qint64 begin = m_fed;
        const qint64 end = begin + data.size();
        m_fed = end;

        for (qsizetype k = 0; k < m_indices.size(); ++k) {
            const int index = m_indices.at(k);
            const Segment &segment = m_engine.m_segments.at(index);
            SegmentState &st = m_engine.m_segmentStates[index];

            // the checksums: over the track as the TOC defines it
            const qint64 sumStart = m_sumStarts.at(k);
            const qint64 sumBytes = segment.checksumByteCount();
            for (qsizetype j = 0; j < st.shifted.size(); ++j) {
                const qint64 windowStart = sumStart + qint64(m_engine.m_options.accurateRipShifts.at(j)) * Cdda::BytesPerSample;
                const qint64 from = std::max(begin, windowStart);
                const qint64 to = std::min(end, windowStart + sumBytes);
                if (from < to) {
                    const QByteArrayView window = data.sliced(from - begin, to - from);
                    st.shifted[j].update(window);
                    st.ctdbShifted[j].update(window);
                }
            }

            SegmentResult &r = m_engine.m_result.segments[index];
            if (!r.ctdbHead.isEmpty()) {
                const qint64 range = qint64(m_engine.m_options.ctdbShiftRange) * Cdda::BytesPerSample;
                const qint64 windowStart = sumStart + qint64(segment.ctdbSkipFirst) * Cdda::BytesPerSample;
                const qint64 windowEnd = sumStart + sumBytes - qint64(segment.ctdbSkipLast) * Cdda::BytesPerSample;
                copyOverlap(r.ctdbHead, windowStart - range, begin, data);
                copyOverlap(r.ctdbTail, windowEnd - range, begin, data);
            }

            if (segment.accurateRip) {
                const qint64 from = std::max(begin, sumStart);
                const qint64 to = std::min(end, sumStart + sumBytes);
                if (from < to) {
                    const QByteArrayView window = data.sliced(from - begin, to - from);
                    st.accurateRip.update(window);
                    st.ctdb.update(window);
                }
            }

            // the file
            const qint64 from = std::max(begin, m_starts.at(k));
            const qint64 to = std::min(end, m_starts.at(k) + segment.byteCount());
            if (from >= to)
                continue;
            const QByteArrayView chunk = data.sliced(from - begin, to - from);
            st.crc.update(chunk);
            st.crcNoNull.update(chunk);
            st.peak.update(chunk);
            r.bytesWritten += chunk.size();

            if (m_engine.m_callbacks.write && !m_engine.m_callbacks.write(index, chunk))
                return false;
        }

        for (qsizetype k = 0; k < m_indices.size(); ++k)
            if (!m_notified.at(k) && m_fed >= m_ends.at(k))
                notify(k);
        return true; // bytes beyond the plan (never expected) are dropped
    }

    // end of the run: report segments whose shifted windows reach beyond it
    void finish()
    {
        for (qsizetype k = 0; k < m_indices.size(); ++k)
            if (!m_notified.at(k))
                notify(k);
    }

    int currentSegment() const
    {
        int current = m_indices.value(0, -1);
        for (qsizetype k = 0; k < m_indices.size(); ++k)
            if (m_starts.at(k) < m_fed)
                current = m_indices.at(k);
        return current;
    }

private:
    void notify(qsizetype k)
    {
        m_notified[k] = true;
        m_engine.segmentComplete(m_indices.at(k));
    }

    RipEngine &m_engine;
    QList<int> m_indices;
    QList<qint64> m_starts; // byte position of each segment in the run
    QList<bool> m_notified;
    qint64 m_skip = 0;
    qint64 m_fed = 0;
    QList<qint64> m_sumStarts; // stream position of each checksum range
    QList<qint64> m_ends; // stream position at which each segment is complete
};

// ---------------------------------------------------------------------------

int cacheDefeatTarget(ReadableArea area, int lba, int count, int index, int reads, int distance, int sectors, bool *fits)
{
    const int afterFirst = lba + count + distance; // preferred: read-ahead moves away from the range
    const int beforeFirst = lba - distance - sectors;
    const int afterRoom = std::max(0, area.endLba - sectors - afterFirst + 1);
    const int beforeRoom = std::max(0, beforeFirst - area.firstLba + 1);

    if (afterRoom + beforeRoom == 0) {
        if (fits)
            *fits = false;
        const int roomAfter = (area.endLba - sectors) - (lba + count);
        const int roomBefore = lba - (area.firstLba + sectors);
        return std::max(roomAfter >= roomBefore ? area.endLba - sectors : area.firstLba, area.firstLba);
    }

    auto positions = [](int room, int step) {
        return room > 0 ? (room - 1) / step + 1 : 0;
    };
    reads = std::max(1, reads);
    int step = std::max(1, distance);
    if (positions(afterRoom, step) + positions(beforeRoom, step) < reads)
        step = std::max(1, (afterRoom + beforeRoom) / reads); // closer together, but still distinct
    if (fits)
        *fits = step >= distance;

    const int afterSlots = positions(afterRoom, step);
    const int beforeSlots = positions(beforeRoom, step);
    int k = index % (afterSlots + beforeSlots);
    for (int i = 0;; ++i) {
        if (i < afterSlots && k-- == 0)
            return afterFirst + i * step;
        if (i < beforeSlots && k-- == 0)
            return beforeFirst - i * step;
    }
}

RipEngine::RipEngine(SectorReader &reader, ReadableArea area, RipOptions options, RipCallbacks callbacks)
    : m_reader(reader)
    , m_area(area)
    , m_options(options)
    , m_callbacks(std::move(callbacks))
{
    m_options.readErrorRetries = std::max(0, m_options.readErrorRetries);
    m_options.secureWindow = std::max(1, m_options.secureWindow);
    m_options.readsPerRound = std::max(1, m_options.readsPerRound);
    m_options.requiredMatches = std::clamp(m_options.requiredMatches, 1, m_options.readsPerRound);
    m_options.maxRounds = std::max(1, m_options.maxRounds);
    m_options.cacheDefeatDistance = std::max(0, m_options.cacheDefeatDistance);
    m_options.cacheDefeatSectors = std::max(1, m_options.cacheDefeatSectors);
    m_options.cacheDefeatReads = std::max(1, m_options.cacheDefeatReads);
    m_options.ctdbShiftRange = std::max(0, m_options.ctdbShiftRange);
}

RipResult RipEngine::run(const QList<Segment> &segments)
{
    QElapsedTimer timer;
    timer.start();

    m_result = RipResult();
    m_segments = segments;
    m_segmentStates.clear();
    m_aborted = false;
    m_doneSectors = 0;
    m_totalSectors = 0;

    // --- validate ---
    for (int i = 0; i < m_segments.size(); ++i) {
        const Segment &s = m_segments.at(i);
        if (s.lastLba < s.firstLba) {
            m_result.error = u"Segment %1 has an empty range"_s.arg(i);
            return m_result;
        }
        if (i > 0 && s.firstLba <= m_segments.at(i - 1).lastLba) {
            m_result.error = u"Segments must be sorted and must not overlap"_s;
            return m_result;
        }
    }

    m_result.segments.resize(m_segments.size());
    m_segmentStates.resize(m_segments.size());
    for (int i = 0; i < m_segments.size(); ++i)
        resetSegment(i);
    m_retry.clear();
    m_forceRetry.clear();

    // --- reader setup ---
    m_c2 = m_options.useC2 && m_reader.supportsC2();
    if (m_options.useC2 && !m_c2)
        log(LogLevel::Warning, u"C2 error pointers requested, but not supported by the drive. Ignoring."_s);
    m_reader.setC2Enabled(m_c2);

    // C2 data enlarges each sector, so ask for the burst size afterwards
    const int maxBurst = std::max(1, m_reader.maxSectorsPerRead());
    m_burst = m_options.burstSectors > 0 ? std::min(m_options.burstSectors, maxBurst) : maxBurst;

    // always set: 0 (maximum) also replaces a speed left over by an earlier rip
    if (!m_reader.setReadSpeed(m_options.readSpeed) && m_options.readSpeed > 0)
        log(LogLevel::Warning, u"Unable to set read speed to %1x."_s.arg(m_options.readSpeed));
    m_recoverySpeedActive = false;

    const bool verifyFirst = m_options.verifyFirst && m_options.mode == ReadMode::Secure && m_callbacks.verify && m_callbacks.discard;

    // --- group contiguous segments into runs ---
    // verifyFirst: a segment without checksums (hidden track one audio) can
    // never be confirmed, so it gets a run of its own that is read securely
    // right away instead of being read fast, discarded and read again
    QList<QList<int>> runs;
    for (int i = 0; i < m_segments.size(); ++i) {
        const bool contiguous = i > 0 && m_segments.at(i).firstLba == m_segments.at(i - 1).lastLba + 1;
        const bool sameKind = i > 0 && m_segments.at(i).accurateRip == m_segments.at(i - 1).accurateRip;
        if (!contiguous || (verifyFirst && !sameKind))
            runs.append(QList<int>());
        runs.last().append(i);
    }
    for (const QList<int> &run : runs) {
        const auto [from, to] = runRange(run);
        m_totalSectors += makeReadPlan(from, to, m_options.readOffset).sectorCount();
    }

    log(LogLevel::Debug,
        u"Extraction: %1 segment(s) in %2 run(s), %3 sectors, burst %4, C2 %5"_s.arg(m_segments.size())
            .arg(runs.size())
            .arg(m_totalSectors)
            .arg(m_burst)
            .arg(m_c2 ? u"on"_s : u"off"_s));

    for (const QList<int> &run : runs) {
        m_verifying = verifyFirst && m_segments.at(run.first()).accurateRip;
        if (!extractRun(run, m_verifying ? ReadMode::Fast : m_options.mode))
            break;
    }
    // verifyFirst: segments the checksums did not confirm are extracted again securely
    m_verifying = false;
    for (int i = 0; i < m_retry.size() && !m_aborted && !canceled(); ++i) {
        resetSegment(m_retry.at(i));
        reportStatus(m_retry.at(i), SegmentStatus::Rereading);
        if (!extractRun({m_retry.at(i)}, ReadMode::Secure))
            break;
    }

    setErrorSpeed(false);
    if (m_c2)
        m_reader.setC2Enabled(false);

    // --- results ---
    for (int i = 0; i < m_segments.size(); ++i)
        updateChecksums(i);

    m_result.canceled = !m_aborted && canceled();
    m_result.completed = !m_aborted && !m_result.canceled;
    m_result.statistics.elapsedMs = timer.elapsed();
    return m_result;
}

bool RipEngine::extractRun(const QList<int> &segmentIndices, ReadMode mode)
{
    m_currentRun = segmentIndices;
    const Segment &first = m_segments.at(segmentIndices.first());
    const Segment &last = m_segments.at(segmentIndices.last());
    const auto [from, to] = runRange(segmentIndices);
    const ReadPlan plan = makeReadPlan(from, to, m_options.readOffset);

    log(LogLevel::Debug,
        u"Run LBA %1..%2 -> drive sectors %3..%4, skip %5 bytes, %6 bytes output"_s.arg(first.firstLba)
            .arg(last.lastLba)
            .arg(plan.firstSector)
            .arg(plan.lastSector)
            .arg(plan.skipBytes)
            .arg(plan.outputBytes));

    Stream stream(*this, segmentIndices, plan, qint64(first.firstLba - from) * SectorBytes);
    const bool ok = mode == ReadMode::Fast ? extractFast(plan, stream) : extractSecure(plan, stream);
    if (ok && !m_aborted && !canceled())
        stream.finish();
    return ok && !m_aborted;
}

int RipEngine::shiftMarginSectors() const
{
    int maxShift = m_options.ctdbShiftRange;
    for (int shift : std::as_const(m_options.accurateRipShifts))
        maxShift = std::max(maxShift, std::abs(shift));
    return (maxShift + Cdda::SamplesPerSector - 1) / Cdda::SamplesPerSector;
}

std::pair<int, int> RipEngine::runRange(const QList<int> &segmentIndices) const
{
    int first = std::numeric_limits<int>::max();
    int last = std::numeric_limits<int>::min();
    for (int index : segmentIndices) {
        const Segment &s = m_segments.at(index);
        first = std::min({first, s.firstLba, s.checksumFirst()});
        last = std::max({last, s.lastLba, s.checksumLast()});
    }
    const int margin = shiftMarginSectors();
    return {first - margin, last + margin};
}

void RipEngine::resetSegment(int index)
{
    const Segment &s = m_segments.at(index);
    const qint64 samples = qint64(s.checksumLast() - s.checksumFirst() + 1) * Cdda::SamplesPerSector;
    SegmentState st;
    st.accurateRip = AccurateRipChecksum(samples, s.accurateRipFirst, s.accurateRipLast);
    st.ctdb = ClippedCrc32(samples, s.ctdbSkipFirst, s.ctdbSkipLast);
    if (s.accurateRip) {
        for (qsizetype j = 0; j < m_options.accurateRipShifts.size(); ++j) {
            st.shifted.append(AccurateRipChecksum(samples, s.accurateRipFirst, s.accurateRipLast));
            st.ctdbShifted.append(st.ctdb);
        }
    }
    m_segmentStates[index] = st;
    SegmentResult r;
    r.segment = s;
    if (s.accurateRip && m_options.ctdbShiftRange > 0) {
        r.ctdbHead = QByteArray(qsizetype(2) * m_options.ctdbShiftRange * Cdda::BytesPerSample, '\0');
        r.ctdbTail = r.ctdbHead;
    }
    m_result.segments[index] = r;
}

void RipEngine::updateChecksums(int index)
{
    SegmentResult &r = m_result.segments[index];
    const SegmentState &st = m_segmentStates.at(index);
    r.crc32 = st.crc.value();
    r.crc32NoNull = st.crcNoNull.value();
    r.peakPercent = st.peak.percent();
    if (!m_segments.at(index).accurateRip)
        return;
    r.accurateRipV1 = st.accurateRip.v1();
    r.accurateRipV2 = st.accurateRip.v2();
    r.accurateRipFrame450 = st.accurateRip.frame450();
    r.ctdbCrc = st.ctdb.value();
    r.accurateRipShifted.clear();
    for (qsizetype j = 0; j < st.shifted.size(); ++j)
        r.accurateRipShifted.append(
            ShiftedChecksums{m_options.accurateRipShifts.at(j), st.shifted.at(j).v1(), st.shifted.at(j).v2(), st.ctdbShifted.at(j).value()});
}

void RipEngine::segmentComplete(int index)
{
    updateChecksums(index);
    SegmentResult &r = m_result.segments[index];
    if (!m_verifying) {
        // secure extraction after verifyFirst: the checksums may confirm it now
        if (!r.suspicious.isEmpty())
            reportStatus(index, SegmentStatus::Suspicious);
        else if (m_options.verifyFirst && m_callbacks.verify && m_retry.contains(index) && m_callbacks.verify(index, r))
            reportStatus(index, SegmentStatus::Confirmed);
        else
            reportStatus(index, SegmentStatus::Done);
        return;
    }
    const int track = r.segment.trackNumber;
    const bool forced = m_forceRetry.contains(index);
    if (!forced && m_callbacks.verify(index, r)) {
        r.acceptedAfterOnePass = true;
        log(LogLevel::Info, u"Track %1 confirmed after one read."_s.arg(track));
        reportStatus(index, SegmentStatus::Confirmed);
        return;
    }
    if (!m_callbacks.discard(index)) {
        m_aborted = true;
        m_result.error = u"Track %1 could not be discarded for a secure extraction"_s.arg(track);
        log(LogLevel::Error, m_result.error);
        return;
    }
    if (forced)
        log(LogLevel::Info, u"Track %1 holds audio the checksums of another track did not confirm; it is extracted again in secure mode."_s.arg(track));
    else
        log(LogLevel::Info, u"Track %1 not confirmed after one read; it is extracted again in secure mode."_s.arg(track));
    scheduleRetry(index);

    // Other files holding part of this checksum range (a pre-gap kept with
    // its own track) are not confirmed either: read them again as well
    const Segment &s = r.segment;
    for (int other : std::as_const(m_currentRun)) {
        const Segment &o = m_segments.at(other);
        if (other == index || m_retry.contains(other) || o.lastLba < s.checksumFirst() || o.firstLba > s.checksumLast())
            continue;
        SegmentResult &confirmed = m_result.segments[other];
        if (!confirmed.acceptedAfterOnePass) {
            m_forceRetry.insert(other); // decided when it is complete
            continue;
        }
        confirmed.acceptedAfterOnePass = false;
        if (!m_callbacks.discard(other)) {
            m_aborted = true;
            m_result.error = u"Track %1 could not be discarded for a secure extraction"_s.arg(o.trackNumber);
            log(LogLevel::Error, m_result.error);
            return;
        }
        log(LogLevel::Info,
            u"Track %1 holds audio the checksums of track %2 did not confirm; it is extracted again in secure mode."_s.arg(o.trackNumber).arg(track));
        scheduleRetry(other);
    }
}

void RipEngine::scheduleRetry(int index)
{
    m_retry.append(index);
    reportStatus(index, SegmentStatus::Unconfirmed);
    const auto [from, to] = runRange({index});
    m_totalSectors += makeReadPlan(from, to, m_options.readOffset).sectorCount();
}

// ---------------------------------------------------------------------------

RipEngine::SectorKind RipEngine::kindOf(int lba) const
{
    if (m_area.contains(lba))
        return SectorKind::Readable;
    if (!m_options.overread)
        return SectorKind::Padding;
    if (lba < m_area.firstLba)
        return m_leadInUnreadable ? SectorKind::Padding : SectorKind::Overread;
    return m_leadOutUnreadable ? SectorKind::Padding : SectorKind::Overread;
}

int RipEngine::homogeneousLength(int lba, int maxCount) const
{
    const SectorKind kind = kindOf(lba);
    int n = 1;
    while (n < maxCount && kindOf(lba + n) == kind)
        ++n;
    return n;
}

SectorReadResult RipEngine::readRaw(int lba, int count)
{
    const int timeouts = m_reader.commandTimeouts();
    SectorReadResult r = m_reader.read(lba, count);
    ++m_result.statistics.readCommands;
    m_result.statistics.sectorsRequested += count;
    bool delivered = false;
    for (const SectorStatus &s : std::as_const(r.sectors)) {
        delivered = delivered || s.ok;
        if (s.recovered)
            ++m_result.statistics.recoveredByDrive;
    }

    // a read error is an answer; only timeouts without any data count
    if (m_reader.commandTimeouts() > timeouts && !delivered)
        ++m_timeoutsInRow;
    else
        m_timeoutsInRow = 0;
    if (m_options.maxTimeoutsInRow > 0 && m_timeoutsInRow >= m_options.maxTimeoutsInRow && !m_aborted) {
        m_aborted = true; // canceled() stops everything from here on
        m_result.driveStalled = true;
        m_result.error = u"The drive stopped answering: %1 read commands in a row timed out"_s.arg(m_timeoutsInRow);
        log(LogLevel::Error, m_result.error);
    }
    return r;
}

RipEngine::Copy RipEngine::acquire(int lba, int count, const QList<bool> *needed)
{
    Copy c;
    c.lba = lba;
    c.audio = QByteArray(qsizetype(count) * SectorBytes, '\0');
    c.status.resize(count);
    c.padded.fill(false, count);

    int i = 0;
    while (i < count && !canceled()) {
        if (needed && !needed->at(i)) {
            ++i; // the caller knows this one has to be recovered anyway
            continue;
        }
        int room = std::min(m_burst, count - i);
        if (needed) {
            int r = 1;
            while (r < room && needed->at(i + r))
                ++r;
            room = r;
        }
        const int n = homogeneousLength(lba + i, room);
        const SectorKind kind = kindOf(lba + i);

        if (kind == SectorKind::Padding) {
            for (int j = 0; j < n; ++j) {
                c.status[i + j].ok = true;
                c.padded[i + j] = true;
            }
        } else {
            const SectorReadResult r = readRaw(lba + i, n);
            std::memcpy(c.audio.data() + qsizetype(i) * SectorBytes, r.audio.constData(), qsizetype(n) * SectorBytes);
            for (int j = 0; j < n; ++j)
                c.status[i + j] = r.sectors.value(j);

            // One failed overread ends overreading on this side for the rip,
            // whatever the cause: every pass then delivers the same silence
            // there (secure mode compares them), not real audio in one pass
            // and silence in the next. It is at most the offset's few hundred
            // samples at the disc edges, which AccurateRip and the CTDB leave
            // out of their checksums anyway.
            if (kind == SectorKind::Overread && !r.allOk()) {
                for (int j = 0; j < n; ++j)
                    dropOverread(c, i + j, r.error);
            }
        }
        i += n;
    }
    return c;
}

// The drive cannot overread reliably on this side: silence from now on.
void RipEngine::dropOverread(Copy &copy, int index, const QString &reason)
{
    const bool leadIn = copy.lba + index < m_area.firstLba;
    (leadIn ? m_leadInUnreadable : m_leadOutUnreadable) = true;
    if (!m_overreadWarningShown) {
        log(LogLevel::Warning, u"Drive cannot read into the %1 (%2). Using silence instead."_s.arg(leadIn ? u"lead-in"_s : u"lead-out"_s, reason));
        m_overreadWarningShown = true;
    }
    std::memset(copy.audio.data() + qsizetype(index) * SectorBytes, 0, SectorBytes);
    copy.status[index] = SectorStatus{true, false, 0};
    copy.padded[index] = true;
}

void RipEngine::defeatCache(int lba, int count)
{
    if (!m_options.cacheDefeat)
        return;

    const int distance = m_options.cacheDefeatDistance;
    const int n = m_options.cacheDefeatSectors;
    const int reads = m_options.cacheDefeatReads;
    for (int i = 0; i < reads; ++i) {
        bool fits = true;
        const int target = cacheDefeatTarget(m_area, lba, count, i, reads, distance, n, &fits);
        if (!fits && !m_shortDiscWarningShown) {
            log(LogLevel::Warning,
                u"Audio area is too short for %1 cache defeat reads %2 sectors apart; re-reads may be served from the drive cache."_s.arg(reads).arg(distance));
            m_shortDiscWarningShown = true;
        }
        readRaw(target, n);
    }
    ++m_result.statistics.cacheDefeats;
}

bool RipEngine::trustworthy(const SectorStatus &status) const
{
    return status.ok && (!m_c2 || status.c2Errors == 0);
}

void RipEngine::setErrorSpeed(bool recovering)
{
    if (m_options.errorReadSpeed <= 0 || recovering == m_recoverySpeedActive)
        return;
    m_reader.setReadSpeed(recovering ? m_options.errorReadSpeed : m_options.readSpeed);
    m_recoverySpeedActive = recovering;
}

bool RipEngine::emitCopy(const Copy &copy, Stream &stream)
{
    for (int i = 0; i < copy.padded.size(); ++i) {
        if (copy.padded.at(i)) {
            for (int seg : segmentsForDriveSector(copy.lba + i))
                ++m_result.segments[seg].paddedSectors;
        }
    }
    if (!stream.feed(copy.audio)) {
        if (canceled())
            return false; // canceled while waiting for the encoders
        m_aborted = true;
        m_result.error = u"Writing the extracted audio failed"_s;
        log(LogLevel::Error, m_result.error);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Fast mode

bool RipEngine::extractFast(const ReadPlan &plan, Stream &stream)
{
    int s = plan.firstSector;
    while (s <= plan.lastSector) {
        if (canceled())
            return false;

        const int n = std::min(m_burst, plan.lastSector - s + 1);
        Copy c = acquire(s, n);

        for (int i = 0; i < n && !canceled(); ++i) {
            if (c.padded.at(i) || trustworthy(c.status.at(i)))
                continue;
            if (!m_area.contains(s + i)) {
                dropOverread(c, i, u"C2 errors"_s);
                continue;
            }
            setErrorSpeed(true);
            recoverFast(s + i, c, i);
        }
        setErrorSpeed(false);

        if (canceled())
            return false;
        if (!emitCopy(c, stream))
            return false;

        s += n;
        m_doneSectors += n;
        reportProgress(stream.currentSegment());
    }
    return true;
}

void RipEngine::recoverFast(int lba, Copy &copy, int index)
{
    noteReread(lba);

    char *dst = copy.audio.data() + qsizetype(index) * SectorBytes;
    bool haveUsable = copy.status.at(index).ok;
    quint16 bestC2 = haveUsable ? copy.status.at(index).c2Errors : 0xFFFF;
    QString lastError = u"read error"_s;

    for (int attempt = 1; attempt <= m_options.readErrorRetries && !canceled(); ++attempt) {
        defeatCache(lba, 1);
        const SectorReadResult r = readRaw(lba, 1);
        const SectorStatus st = r.sectors.value(0);
        if (!st.ok) {
            if (!r.error.isEmpty())
                lastError = r.error;
            continue;
        }
        if (trustworthy(st)) {
            std::memcpy(dst, r.audio.constData(), SectorBytes);
            copy.status[index] = st;
            log(LogLevel::Info, u"Sector %1 recovered after %2 retr%3."_s.arg(lba).arg(attempt).arg(attempt == 1 ? u"y"_s : u"ies"_s));
            return;
        }
        if (st.c2Errors < bestC2) {
            std::memcpy(dst, r.audio.constData(), SectorBytes);
            bestC2 = st.c2Errors;
            haveUsable = true;
            copy.status[index] = st;
        }
    }

    if (haveUsable) {
        markSuspicious(lba, false, u"C2 errors reported in every copy (best: %1 bytes)"_s.arg(bestC2));
    } else {
        std::memset(dst, 0, SectorBytes);
        copy.status[index] = SectorStatus{};
        markSuspicious(lba, true, u"unreadable after %1 retries (%2)"_s.arg(m_options.readErrorRetries).arg(lastError));
    }
}

// ---------------------------------------------------------------------------
// Secure mode

bool RipEngine::extractSecure(const ReadPlan &plan, Stream &stream)
{
    int w = plan.firstSector;
    while (w <= plan.lastSector) {
        if (canceled())
            return false;

        const int n = std::min(m_options.secureWindow, plan.lastSector - w + 1);

        Copy a = acquire(w, n);
        QList<bool> accepted(n, false);

        if (m_c2) {
            for (int i = 0; i < n; ++i)
                accepted[i] = a.padded.at(i) || trustworthy(a.status.at(i));
        } else {
            defeatCache(w, n);
            // A sector the drive did not deliver in the first pass cannot be
            // confirmed by a second one; it goes to the error recovery in any
            // case, so the second pass leaves it out.
            QList<bool> needed(n);
            for (int i = 0; i < n; ++i)
                needed[i] = a.status.at(i).ok;
            const Copy b = acquire(w, n, &needed);
            if (canceled())
                return false;
            for (int i = 0; i < n; ++i) {
                if (a.padded.at(i)) {
                    accepted[i] = true;
                } else if (a.status.at(i).ok && b.status.at(i).ok) {
                    accepted[i] = a.sector(i) == b.sector(i);
                    if (!accepted.at(i))
                        ++m_result.statistics.passMismatches;
                }
            }
        }

        // Overread data that is not confirmed is not worth an error recovery
        for (int i = 0; i < n; ++i) {
            if (!accepted.at(i) && !m_area.contains(w + i)) {
                dropOverread(a, i, m_c2 ? u"C2 errors"_s : u"copies differ"_s);
                accepted[i] = true;
            }
        }

        int i = 0;
        while (i < n && !canceled()) {
            if (accepted.at(i)) {
                ++i;
                continue;
            }
            int j = i;
            while (j < n && !accepted.at(j) && j - i < m_burst)
                ++j;
            setErrorSpeed(true);
            resolveSecure(w + i, j - i, a, i);
            i = j;
        }
        setErrorSpeed(false);

        if (canceled())
            return false;
        if (!emitCopy(a, stream))
            return false;

        w += n;
        m_doneSectors += n;
        reportProgress(stream.currentSegment());
    }
    return true;
}

void RipEngine::resolveSecure(int lba, int count, Copy &result, int offset)
{
    struct Candidate {
        QByteArray data;
        int votes = 0;
        quint16 minC2 = 0xFFFF;
    };

    auto vote = [](QList<Candidate> &list, QByteArrayView data, quint16 c2) -> Candidate & {
        for (Candidate &c : list) {
            if (QByteArrayView(c.data) == data) {
                ++c.votes;
                c.minC2 = std::min(c.minC2, c2);
                return c;
            }
        }
        list.append(Candidate{data.toByteArray(), 1, c2});
        return list.last();
    };

    auto store = [&](int j, QByteArrayView data, const SectorStatus &st) {
        std::memcpy(result.audio.data() + qsizetype(offset + j) * SectorBytes, data.constData(), SectorBytes);
        result.status[offset + j] = st;
    };

    for (int j = 0; j < count; ++j)
        noteReread(lba + j);

    QList<QList<Candidate>> overall(count);
    QList<bool> done(count, false); // resolved, data is in `result`
    QList<bool> open(count, true); // still worth reading
    QList<int> failed(count, 0); // reads that returned nothing for this sector
    int remaining = count;
    int reads = 0;
    int givenUp = 0;

    // A sector the drive did not deliver in the pass before fails the whole
    // command again, and the reader then reads every sector of the command
    // singly: one bad sector costs a command for each of its neighbours.
    // Ask for the sectors we still need one by one instead.
    bool single = false;
    for (int j = 0; j < count; ++j)
        if (!result.status.at(offset + j).ok)
            single = true;

    auto close = [&](int j, bool resolved) {
        open[j] = false;
        done[j] = resolved;
        --remaining;
        if (!resolved)
            ++givenUp;
    };

    auto take = [&](int j, const SectorReadResult &r, int index, QList<QList<Candidate>> &thisRound) {
        const SectorStatus st = r.sectors.value(index);
        if (!st.ok) {
            ++failed[j];
            // Not a single copy so far: on a real drive every one of these
            // reads costs seconds, and the rounds that are left would spend
            // them without ever seeing data. Whoever wants more sets the
            // retries higher.
            if (overall.at(j).isEmpty() && failed.at(j) >= m_options.readErrorRetries)
                close(j, false);
            return;
        }
        const QByteArrayView data = r.sector(index);
        vote(overall[j], data, st.c2Errors);
        if (m_c2) {
            if (st.c2Errors == 0) {
                store(j, data, st);
                close(j, true);
            }
            return;
        }
        const Candidate &c = vote(thisRound[j], data, st.c2Errors);
        if (c.votes >= m_options.requiredMatches) {
            store(j, data, st);
            close(j, true);
        }
    };

    for (int round = 0; round < m_options.maxRounds && remaining > 0; ++round) {
        QList<QList<Candidate>> thisRound(count);
        for (int k = 0; k < m_options.readsPerRound && remaining > 0; ++k) {
            if (canceled())
                return;

            // Sectors that are done leave the range: the drive is only asked
            // for the part that is still open.
            int first = 0;
            while (first < count && !open.at(first))
                ++first;
            int last = count - 1;
            while (last > first && !open.at(last))
                --last;

            defeatCache(lba + first, last - first + 1);
            ++reads;

            if (single) {
                for (int j = first; j <= last; ++j) {
                    if (!open.at(j))
                        continue;
                    if (canceled())
                        return;
                    take(j, readRaw(lba + j, 1), 0, thisRound);
                }
            } else {
                const SectorReadResult r = readRaw(lba + first, last - first + 1);
                if (r.splitIntoSingleReads)
                    single = true; // the drive will not read this range in one command
                for (int j = first; j <= last; ++j)
                    if (open.at(j))
                        take(j, r, j - first, thisRound);
            }
        }
    }

    int resolved = 0;
    for (int j = 0; j < count; ++j)
        if (done.at(j))
            ++resolved;

    for (int j = 0; j < count; ++j) {
        if (done.at(j))
            continue;
        const QList<Candidate> &cands = overall.at(j);
        if (cands.isEmpty()) {
            std::memset(result.audio.data() + qsizetype(offset + j) * SectorBytes, 0, SectorBytes);
            result.status[offset + j] = SectorStatus{};
            markSuspicious(lba + j, true, u"unreadable in %1 attempts"_s.arg(failed.at(j)));
            continue;
        }
        auto best = std::max_element(cands.cbegin(), cands.cend(), [](const Candidate &x, const Candidate &y) {
            if (x.votes != y.votes)
                return x.votes < y.votes;
            return x.minC2 > y.minC2;
        });
        store(j, best->data, SectorStatus{true, false, best->minC2});
        markSuspicious(lba + j,
                       false,
                       u"no reliable copy: best copy seen %1 times in %2 reads, %3 different copies"_s.arg(best->votes).arg(reads).arg(cands.size()));
    }

    log(LogLevel::Info,
        u"Error recovery for sectors %1..%2: %3 of %4 resolved after %5 reads%6"_s.arg(lba)
            .arg(lba + count - 1)
            .arg(resolved)
            .arg(count)
            .arg(reads)
            .arg(givenUp > 0 ? u", %1 given up (no data at all)"_s.arg(givenUp) : QString()));
}

// ---------------------------------------------------------------------------

// With a read offset a drive sector carries samples of two disc sectors, which
// may belong to two tracks: every segment it overlaps is affected.
QList<int> RipEngine::segmentsForDriveSector(int driveSector) const
{
    QList<int> result;
    if (m_currentRun.isEmpty())
        return result;
    const qint64 first = qint64(driveSector) * Cdda::SamplesPerSector - m_options.readOffset; // disc samples
    const qint64 end = first + Cdda::SamplesPerSector;
    for (int index : m_currentRun) {
        const Segment &s = m_segments.at(index);
        if (first < qint64(s.lastLba + 1) * Cdda::SamplesPerSector && end > qint64(s.firstLba) * Cdda::SamplesPerSector)
            result.append(index);
    }
    return result; // empty for the margin around a run
}

void RipEngine::markSuspicious(int driveSector, bool zeroFilled, const QString &reason)
{
    const int lba = correctedLbaForDriveSector(driveSector, m_options.readOffset);
    for (int seg : segmentsForDriveSector(driveSector)) {
        const Segment &s = m_segments.at(seg);
        SuspiciousPosition p;
        p.lba = std::clamp(lba, s.firstLba, s.lastLba);
        p.zeroFilled = zeroFilled;
        p.reason = reason;
        m_result.segments[seg].suspicious.append(p);
    }
    log(LogLevel::Warning, u"Suspicious position at LBA %1 (drive sector %2): %3"_s.arg(lba).arg(driveSector).arg(reason));
}

void RipEngine::noteReread(int driveSector)
{
    for (int seg : segmentsForDriveSector(driveSector))
        ++m_result.segments[seg].rereadSectors;
}

bool RipEngine::canceled() const
{
    return m_aborted || (m_callbacks.isCanceled && m_callbacks.isCanceled());
}

void RipEngine::log(LogLevel level, const QString &message) const
{
    if (m_callbacks.log)
        m_callbacks.log(level, message);
}

void RipEngine::reportProgress(int segmentIndex)
{
    if (!m_callbacks.progress)
        return;
    const qint64 segmentSectors = segmentIndex >= 0 ? m_result.segments.at(segmentIndex).bytesWritten / SectorBytes : 0;
    m_callbacks.progress(m_doneSectors, m_totalSectors, segmentIndex, segmentSectors);
}

void RipEngine::reportStatus(int segmentIndex, SegmentStatus status)
{
    if (m_callbacks.segmentStatus)
        m_callbacks.segmentStatus(segmentIndex, status);
}

// ---------------------------------------------------------------------------

void keepPregapsWithTrack(QList<Segment> &segments, const QMap<int, int> &index00, int firstAudioTrack)
{
    for (Segment &s : segments) {
        if (s.trackNumber == 0) // hidden track one audio
            continue;
        const int first = s.checksumFirst();
        const int last = s.checksumLast();
        const int own = index00.value(s.trackNumber, -1);
        const int next = index00.value(s.trackNumber + 1, -1);
        int fileFirst = first;
        int fileLast = last;
        if (s.trackNumber != firstAudioTrack && own >= 0 && own < first)
            fileFirst = own;
        if (next > first && next <= last)
            fileLast = next - 1; // the rest is the next track's pre-gap
        if (fileFirst == first && fileLast == last)
            continue;
        s.checksumFirstLba = first;
        s.checksumLastLba = last;
        s.firstLba = fileFirst;
        s.lastLba = fileLast;
    }
}

}
