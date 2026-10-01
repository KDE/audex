/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "simulatedsectorreader.h"

#include "core/cdg.h"
#include "core/subchannel.h"

#include <algorithm>
#include <cstring>

using namespace Qt::StringLiterals;

namespace Audex::Sim
{

using Cdda::BytesPerSample;
using Cdda::SamplesPerSector;
using Cdda::SectorBytes;

// ---------------------------------------------------------------------------

SimulatedDisc SimulatedDisc::generate(const QList<int> &trackSectors, int htoaSectors, quint32 seed, bool enhancedCd)
{
    SimulatedDisc disc;
    int lba = htoaSectors;
    int number = 1;
    for (int sectors : trackSectors) {
        Cdda::Track t;
        t.number = number++;
        t.firstLba = lba;
        t.lastLba = lba + sectors - 1;
        disc.toc.tracks.append(t);
        lba += sectors;
    }
    const int audioEnd = lba;
    disc.toc.leadOutLba = audioEnd;

    if (enhancedCd) {
        Cdda::Track data;
        data.number = number;
        data.session = 2;
        data.audio = false;
        data.firstLba = audioEnd + Cdda::EnhancedCdSessionGap;
        data.lastLba = data.firstLba + 3000 - 1;
        disc.toc.tracks.append(data);
        disc.toc.leadOutLba = data.lastLba + 1;
    }
    disc.toc.source = u"simulation"_s;

    // Pseudo random "audio"; values do not matter, uniqueness does.
    const qsizetype bytes = qsizetype(audioEnd) * SectorBytes;
    QList<quint32> words((bytes + 3) / 4);
    QRandomGenerator rng(seed);
    rng.fillRange(words.data(), words.size());
    disc.audio = QByteArray(reinterpret_cast<const char *>(words.constData()), bytes);
    return disc;
}

// ---------------------------------------------------------------------------

SimulatedSectorReader::SimulatedSectorReader(SimulatedDisc disc, DriveModel model)
    : m_disc(std::move(disc))
    , m_model(model)
    , m_rng(model.seed)
{
}

void SimulatedSectorReader::addDefect(int driveLba, Defect defect)
{
    m_defects.insert(driveLba, defect);
}

int SimulatedSectorReader::maxSectorsPerRead() const
{
    return m_model.maxSectorsPerRead;
}

bool SimulatedSectorReader::supportsC2() const
{
    return m_model.c2Capable;
}

void SimulatedSectorReader::setC2Enabled(bool enabled)
{
    m_c2Enabled = enabled && m_model.c2Capable;
}

void SimulatedSectorReader::setProbeMode(bool enabled)
{
    m_probeMode = enabled;
}

bool SimulatedSectorReader::setReadSpeed(int factor)
{
    if (!m_model.speedControl)
        return false;
    m_speed = factor;
    ++m_stats.speedChanges;
    return true;
}

int SimulatedSectorReader::commandTimeouts() const
{
    return int(m_stats.timeouts);
}

bool SimulatedSectorReader::stalled() const
{
    return m_model.stallsAfterTimeouts > 0 && m_stats.timeouts >= m_model.stallsAfterTimeouts;
}

bool SimulatedSectorReader::timesOut(int lba, int count) const
{
    if (stalled())
        return true;
    if (m_model.c2BlockReadsTimeOut && m_c2Enabled && count > 1)
        return true;
    if (m_model.overreadTimesOut)
        for (int i = 0; i < count; ++i)
            if (!isReadable(lba + i))
                return true;
    return false;
}

qint64 SimulatedSectorReader::slowdown(qint64 command) const
{
    return command >= m_model.slowFrom && command < m_model.slowFrom + m_model.slowCommands ? m_model.slowUs : 0;
}

void SimulatedSectorReader::noteTimeout()
{
    ++m_stats.timeouts;
    m_stats.timeoutWaitMs += m_probeMode ? 8000 : 30000; // the timeouts of the MMC reader
    m_cache.clear();
}

Rip::SectorReadResult SimulatedSectorReader::timeout(int lba, int count)
{
    Q_UNUSED(lba)
    noteTimeout();
    return commandFailed(count, u"host adapter error 0x03 (timeout)"_s, (m_probeMode ? 8000 : 30000) * qint64(1000));
}

// a command a real drive aborts as a whole
Rip::SectorReadResult SimulatedSectorReader::commandFailed(int count, const QString &error, qint64 elapsedUs) const
{
    Rip::SectorReadResult r;
    r.audio = QByteArray(qsizetype(count) * SectorBytes, '\0');
    r.sectors.resize(count);
    r.elapsedUs = elapsedUs;
    r.error = error;
    return r;
}

QString SimulatedSectorReader::description() const
{
    return u"Simulated drive (offset %1, cache %2 sectors, overread %3/%4, C2 %5)"_s.arg(m_model.readOffset)
        .arg(m_model.cacheSectors)
        .arg(m_model.leadInReadable ? u"yes"_s : u"no"_s)
        .arg(m_model.leadOutReadable ? u"yes"_s : u"no"_s)
        .arg(m_model.c2Capable ? u"yes"_s : u"no"_s);
}

bool SimulatedSectorReader::isReadable(int lba) const
{
    const int audioEnd = m_disc.toc.audioEndLba();
    if (lba < 0)
        return m_model.leadInReadable && lba >= -1000;
    if (lba >= audioEnd)
        return m_model.leadOutReadable && lba < audioEnd + 6750;
    return true;
}

QByteArray SimulatedSectorReader::pristineSector(int lba, int shiftSamples) const
{
    QByteArray out(SectorBytes, '\0');
    // drive sample j carries disc sample j - offset
    const qint64 firstByte = (qint64(lba) * SamplesPerSector - m_model.readOffset + shiftSamples) * BytesPerSample;
    const qint64 from = std::max<qint64>(firstByte, 0);
    const qint64 to = std::min<qint64>(firstByte + SectorBytes, m_disc.audio.size());
    if (to > from)
        std::memcpy(out.data() + (from - firstByte), m_disc.audio.constData() + from, to - from);
    return out;
}

qint64 SimulatedSectorReader::seekTime(int lba) const
{
    const qint64 distance = std::abs(qint64(lba) - m_head);
    if (distance == 0)
        return 0;
    return std::min<qint64>(150000, 3000 + distance * 5);
}

bool SimulatedSectorReader::physicalRead(int lba, int shiftSamples, Rip::SectorStatus &status, char *out, QString *error)
{
    ++m_stats.physicalSectors;
    status = Rip::SectorStatus{};

    if (!isReadable(lba)) {
        if (error)
            *error = u"ILLEGAL REQUEST: LBA %1 out of range"_s.arg(lba);
        return false;
    }

    if (m_c2Enabled && m_model.c2ReadsFail) {
        if (error)
            *error = u"ILLEGAL REQUEST: invalid field in CDB"_s;
        return false;
    }

    const QByteArray pristine = pristineSector(lba, shiftSamples);
    std::memcpy(out, pristine.constData(), SectorBytes);

    const auto it = m_defects.constFind(lba);
    if (it == m_defects.cend()) {
        status.ok = true;
        return true;
    }

    const Defect &d = it.value();
    const int readIndex = m_defectReads[lba]++;
    const bool hit = d.pattern.isEmpty() ? m_rng.generateDouble() < d.probability : d.pattern.at(readIndex % d.pattern.size());

    switch (d.kind) {
    case DefectKind::Unreadable:
        if (hit) {
            if (error)
                *error = u"MEDIUM ERROR: L-EC uncorrectable error at LBA %1"_s.arg(lba);
            return false;
        }
        break;
    case DefectKind::Noisy:
        if (hit) {
            const int n = 1 + int(m_rng.bounded(8));
            for (int i = 0; i < n; ++i)
                out[m_rng.bounded(SectorBytes)] ^= char(1 + m_rng.bounded(255));
            status.c2Errors = m_c2Enabled ? d.c2Errors : 0;
        }
        break;
    case DefectKind::Wrong: {
        QRandomGenerator fixed(quint32(lba) * 2654435761u + m_model.seed);
        for (int i = 0; i < 4; ++i)
            out[fixed.bounded(SectorBytes)] ^= char(1 + fixed.bounded(255));
        status.c2Errors = m_c2Enabled ? d.c2Errors : 0;
        break;
    }
    }
    status.ok = true;
    return true;
}

Rip::SectorReadResult SimulatedSectorReader::splitIntoSingleReads(int lba, int count, const QString &error, qint64 elapsedUs)
{
    Rip::SectorReadResult single;
    single.audio = QByteArray(qsizetype(count) * SectorBytes, '\0');
    single.sectors.resize(count);
    single.elapsedUs = elapsedUs;
    single.error = error;
    single.splitIntoSingleReads = true;
    for (int i = 0; i < count; ++i) {
        const qint64 timeouts = m_stats.timeouts;
        const Rip::SectorReadResult one = read(lba + i, 1);
        std::memcpy(single.audio.data() + qsizetype(i) * SectorBytes, one.audio.constData(), SectorBytes);
        single.sectors[i] = one.sectors.value(0);
        single.elapsedUs += one.elapsedUs;
        if (single.error.isEmpty())
            single.error = one.error;
        if (m_stats.timeouts > timeouts)
            break; // like the MMC reader: the drive is not answering
    }
    return single;
}

Rip::SectorReadResult SimulatedSectorReader::read(int lba, int count)
{
    const qint64 command = m_stats.commands++;

    Rip::SectorReadResult r;
    r.audio = QByteArray(qsizetype(count) * SectorBytes, '\0');
    r.sectors.resize(count);

    if (timesOut(lba, count)) {
        const Rip::SectorReadResult t = timeout(lba, count);
        return count == 1 || m_probeMode ? t : splitIntoSingleReads(lba, count, t.error, t.elapsedUs);
    }

    // A transfer the drive or its USB bridge cannot do fails as a whole.
    if (count > m_model.maxSectorsPerRead) {
        const QString error = u"ILLEGAL REQUEST: transfer length of %1 sectors is too large"_s.arg(count);
        return m_probeMode ? commandFailed(count, error, 0) : splitIntoSingleReads(lba, count, error, 0);
    }

    // served from cache?
    for (qsizetype k = 0; k < m_cache.size(); ++k) {
        const CacheSegment &segment = m_cache.at(k);
        if (lba < segment.first || lba + count > segment.first + segment.status.size())
            continue;
        ++m_stats.cacheHits;
        const qsizetype off = qsizetype(lba - segment.first) * SectorBytes;
        std::memcpy(r.audio.data(), segment.data.constData() + off, qsizetype(count) * SectorBytes);
        for (int i = 0; i < count; ++i)
            r.sectors[i] = segment.status.at(lba - segment.first + i);
        r.elapsedUs = 300 + 20 * count + slowdown(command);
        m_cache.move(k, 0);
        return r;
    }

    const int speed = m_speed > 0 ? m_speed : 40;
    r.elapsedUs = seekTime(lba) + qint64(count) * 13333 / speed + slowdown(command);

    // Without accurate stream the drive misses the requested position by a
    // few samples whenever the read does not continue the previous one.
    const int jitter = m_model.jitterSamples > 0 && lba != m_head ? int(m_rng.bounded(2 * m_model.jitterSamples + 1)) - m_model.jitterSamples : 0;

    bool failed = false;
    for (int i = 0; i < count; ++i) {
        QString error;
        if (!physicalRead(lba + i, jitter, r.sectors[i], r.audio.data() + qsizetype(i) * SectorBytes, &error)) {
            failed = true;
            if (r.error.isEmpty())
                r.error = error;
        }
    }
    m_head = lba + count;

    if (failed) {
        m_cache.clear();
        if (count == 1) {
            r.audio.fill('\0');
            return r;
        }
        if (m_probeMode)
            return commandFailed(count, r.error, r.elapsedUs);
        // A real drive aborts the whole command. The MMC reader then falls
        // back to single sector reads; emulate exactly that.
        return splitIntoSingleReads(lba, count, r.error, r.elapsedUs);
    }

    // fill the cache: requested sectors plus read-ahead
    if (m_model.cacheSectors > 0) {
        const int total = std::max(count, m_model.cacheSectors);
        CacheSegment segment;
        segment.first = lba;
        segment.data = r.audio;
        segment.status = r.sectors;
        segment.data.reserve(qsizetype(total) * SectorBytes);
        char buffer[SectorBytes];
        for (int i = count; i < total; ++i) {
            Rip::SectorStatus st;
            if (!physicalRead(lba + i, jitter, st, buffer, nullptr))
                break;
            segment.data.append(buffer, SectorBytes);
            segment.status.append(st);
        }
        m_head = lba + int(segment.status.size());
        m_cache.prepend(std::move(segment));
        while (m_cache.size() > std::max(1, m_model.cacheSegments))
            m_cache.removeLast();
    }
    return r;
}

QByteArray SimulatedSectorReader::qFrame(int s)
{
    Cdda::SubQ q;
    q.adr = 1;
    // the track a sector belongs to: its pregap counts as part of it
    const Cdda::Track *track = nullptr;
    for (const Cdda::Track &t : m_disc.toc.tracks)
        if (s >= m_disc.index00.value(t.number, t.firstLba))
            track = &t;
    if (!track && !m_disc.toc.tracks.isEmpty())
        track = &m_disc.toc.tracks.first(); // hidden track one audio: pregap of track 1
    if (s >= m_disc.toc.audioEndLba()) {
        q.track = 0xAA;
        q.index = 1;
    } else {
        q.track = track->number;
        q.control = track->preEmphasis ? 0x1 : 0x0;
        if (s < track->firstLba) {
            q.index = 0;
            q.relative = track->firstLba - s; // counts down to index 01
        } else {
            q.index = 1;
            for (int start : m_disc.indexes.value(track->number))
                if (s >= start)
                    ++q.index;
            q.relative = s - track->firstLba;
        }
        if (s % 100 == 50 && m_disc.isrc.contains(track->number)) {
            q = Cdda::SubQ{};
            q.adr = 3;
            q.isrc = m_disc.isrc.value(track->number);
        } else if (s % 100 == 75 && !m_disc.mcn.isEmpty()) {
            q = Cdda::SubQ{};
            q.adr = 2;
            q.mcn = m_disc.mcn;
        }
    }
    q.absoluteLba = s;
    QByteArray frame = Cdda::formatSubQ(q);
    if (m_model.subchannelErrorRate > 0 && m_rng.generateDouble() < m_model.subchannelErrorRate)
        frame[3] = char(frame.at(3) ^ 0x10); // CRC no longer matches
    return frame;
}

QByteArray SimulatedSectorReader::readSubchannelQ(int lba, int count)
{
    if (!m_model.subchannel)
        return {};
    ++m_stats.commands;
    if (m_model.subchannelTimesOut || stalled()) {
        noteTimeout();
        return {};
    }
    QByteArray out;
    for (int requested = lba; requested < lba + count; ++requested) {
        if (m_model.subchannelUnreadable.contains(requested))
            break; // what was read so far, like a real drive
        out += qFrame(requested + m_model.subchannelShift);
    }
    return out;
}

QByteArray SimulatedSectorReader::readSubchannelRaw(int lba, int count)
{
    if (!m_model.rwSubchannel)
        return {};
    ++m_stats.commands;
    if (stalled()) {
        noteTimeout();
        return {};
    }
    // R-W as on the disc: the packs interleaved (unless the drive undoes it)
    const int sectors = int(m_disc.cdg.size() / Cdda::SubcodeBytes);
    if (m_rwOnDisc.isEmpty() && sectors > 0)
        m_rwOnDisc = Cdda::interleaveCdg(m_disc.cdg);
    const QByteArray &rw = m_model.rwDeinterleaved ? m_disc.cdg : m_rwOnDisc;

    QByteArray out;
    for (int requested = lba; requested < lba + count; ++requested) {
        const int s = requested + m_model.subchannelShift;
        if (!isReadable(s))
            break;
        QByteArray block(Cdda::SubcodeBytes, '\0');
        const QByteArray q = qFrame(s);
        for (int bit = 0; bit < Cdda::SubQBytes * 8; ++bit)
            if (q.at(bit / 8) & (0x80 >> (bit % 8)))
                block[bit] = char(block.at(bit) | 0x40);
        if (s >= 0 && s < sectors)
            for (int i = 0; i < Cdda::SubcodeBytes; ++i)
                block[i] = char(block.at(i) | (rw.at(qsizetype(s) * Cdda::SubcodeBytes + i) & 0x3F));
        if (m_model.rwErrorRate > 0 && m_rng.generateDouble() < m_model.rwErrorRate)
            block[int(m_rng.bounded(Cdda::SubcodeBytes))] ^= char(1 + m_rng.bounded(0x3F));
        out += block;
    }
    return out;
}

// ---------------------------------------------------------------------------

QByteArray expectedAudio(const SimulatedSectorReader &reader, int firstLba, int lastLba, int correctionOffset, bool overread, Rip::ReadableArea area)
{
    const qint64 samples = qint64(lastLba - firstLba + 1) * SamplesPerSector;
    QByteArray out(samples * BytesPerSample, '\0');
    const QByteArrayView disc(reader.disc().audio);
    const int driveOffset = reader.model().readOffset;

    for (qint64 k = 0; k < samples; ++k) {
        const qint64 i = qint64(firstLba) * SamplesPerSector + k; // disc sample
        const qint64 j = i + correctionOffset; // drive sample requested
        const int sector = int(Rip::floorDiv(j, SamplesPerSector));
        const bool available = area.contains(sector) || (overread && reader.isReadable(sector));
        if (!available)
            continue;
        const qint64 source = j - driveOffset; // disc sample the drive delivers
        if (source < 0 || source >= reader.disc().sampleCount())
            continue;
        std::memcpy(out.data() + k * BytesPerSample, disc.constData() + source * BytesPerSample, BytesPerSample);
    }
    return out;
}

}
