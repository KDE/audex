/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QHash>
#include <QMap>
#include <QRandomGenerator>
#include <QSet>

#include "core/ripengine.h"
#include "core/sectorreader.h"
#include "core/toc.h"

// A simulated CD drive for tests and experiments. It models the behaviour
// that matters for secure ripping:
//  - a read offset (samples are delivered shifted)
//  - lead-in / lead-out overread capability
//  - an audio cache with read-ahead (identical data on immediate re-reads)
//  - defects: unreadable sectors, noisy sectors (random corruption per
//    physical read) and consistently wrong sectors (interpolation), optionally
//    flagged by C2 error pointers
//  - virtual timing (seek + read time) for the cache probe

namespace Audex::Sim
{

struct SimulatedDisc {
    Cdda::Toc toc;
    QByteArray audio; // audio area [0, toc.audioEndLba()) as 16 bit stereo PCM

    // Q sub-channel layout
    QMap<int, int> index00; // track -> first sector of its pregap
    QMap<int, QList<int>> indexes; // track -> first sectors of index 02, 03, ...
    QMap<int, QString> isrc;
    QString mcn;

    // CD+G: packs as written (de-interleaved), 96 bytes per sector of the
    // audio area; empty: none
    QByteArray cdg;

    qint64 sampleCount() const
    {
        return audio.size() / Cdda::BytesPerSample;
    }

    // Tracks are contiguous, starting at LBA htoaSectors. With enhancedCd a
    // data track is appended in a second session.
    static SimulatedDisc generate(const QList<int> &trackSectors, int htoaSectors = 0, quint32 seed = 1, bool enhancedCd = false);
};

enum class DefectKind {
    Unreadable, // read fails with the given probability
    Noisy, // data is randomly corrupted with the given probability
    Wrong, // data is always corrupted in the same way (drive interpolation)
};

struct Defect {
    DefectKind kind = DefectKind::Unreadable;
    double probability = 1.0;
    quint16 c2Errors = 0; // reported for corrupted reads if C2 is enabled
    // Deterministic trigger instead of `probability`: physical read n
    // (0-based) of the sector is affected iff pattern[n % pattern.size()].
    // Empty list = random with `probability`. (DefectKind::Wrong is always
    // affected; the pattern only applies to Unreadable and Noisy.)
    QList<bool> pattern;

    Defect(DefectKind kind = DefectKind::Unreadable, double probability = 1.0, quint16 c2Errors = 0, const QList<bool> &pattern = {})
        : kind(kind)
        , probability(probability)
        , c2Errors(c2Errors)
        , pattern(pattern)
    {
    }
};

struct DriveModel {
    int readOffset = 0; // the drive's read offset correction value
    int jitterSamples = 0; // no accurate stream: reads after a seek are shifted
    bool leadInReadable = false;
    bool leadOutReadable = false;
    int cacheSectors = 0; // 0 = no audio cache
    int cacheSegments = 1; // cache segments, least recently used one is replaced
    int maxSectorsPerRead = 27;
    bool c2Capable = false;
    bool c2ReadsFail = false; // advertises C2 pointers, but every read with them fails
    // Commands the drive does not answer: they fail after the SCSI timeout.
    bool c2BlockReadsTimeOut = false; // reads of more than one sector with C2 pointers
    bool overreadTimesOut = false; // reads outside the readable area (instead of ILLEGAL REQUEST)
    bool subchannelTimesOut = false; // READ CD with Q sub-channel
    int stallsAfterTimeouts = 0; // after that many timeouts no command is answered any more (0: never)
    // read commands [slowFrom, slowFrom + slowCommands) take slowUs longer:
    // the drive changes its speed or spins up again
    int slowFrom = -1;
    int slowCommands = 0;
    qint64 slowUs = 0;
    bool subchannel = true; // delivers Q sub-channel data
    double subchannelErrorRate = 0.0; // Q frames with a broken CRC
    int subchannelShift = 0; // sector n delivers the Q frame of sector n + shift
    QSet<int> subchannelUnreadable; // a Q read stops before these sectors (short read)
    bool rwSubchannel = true; // delivers the raw P-W sub-channel
    bool rwDeinterleaved = false; // ... with the R-W packs already de-interleaved
    double rwErrorRate = 0.0; // raw blocks with a wrong R-W symbol, per read
    bool speedControl = true;
    quint32 seed = 42;
};

struct SimulatedStatistics {
    qint64 commands = 0;
    qint64 physicalSectors = 0;
    qint64 cacheHits = 0;
    qint64 speedChanges = 0;
    qint64 timeouts = 0;
    qint64 timeoutWaitMs = 0; // time a real drive would have kept the caller waiting
};

class SimulatedSectorReader : public Rip::SectorReader
{
public:
    SimulatedSectorReader(SimulatedDisc disc, DriveModel model);

    // Defects are addressed in drive sector space (what the drive is asked for).
    void addDefect(int driveLba, Defect defect);

    Rip::SectorReadResult read(int lba, int count) override;
    int maxSectorsPerRead() const override;
    bool supportsC2() const override;
    void setC2Enabled(bool enabled) override;
    void setProbeMode(bool enabled) override;
    bool setReadSpeed(int factor) override;
    QString description() const override;
    QByteArray readSubchannelQ(int lba, int count) override;
    QByteArray readSubchannelRaw(int lba, int count) override;
    int commandTimeouts() const override;

    const SimulatedStatistics &statistics() const
    {
        return m_stats;
    }
    const SimulatedDisc &disc() const
    {
        return m_disc;
    }
    const DriveModel &model() const
    {
        return m_model;
    }

    bool isReadable(int lba) const;
    QByteArray pristineSector(int lba, int shiftSamples = 0) const; // what a perfect drive with this offset returns

private:
    bool physicalRead(int lba, int shiftSamples, Rip::SectorStatus &status, char *out, QString *error);
    Rip::SectorReadResult splitIntoSingleReads(int lba, int count, const QString &error, qint64 elapsedUs);
    bool stalled() const;
    QByteArray qFrame(int sector); // formatted Q of a sector (with the error rate)
    bool timesOut(int lba, int count) const;
    Rip::SectorReadResult timeout(int lba, int count);
    void noteTimeout();
    qint64 slowdown(qint64 command) const;
    Rip::SectorReadResult commandFailed(int count, const QString &error, qint64 elapsedUs) const;
    qint64 seekTime(int lba) const;

    SimulatedDisc m_disc;
    DriveModel m_model;
    QHash<int, Defect> m_defects;
    QHash<int, int> m_defectReads; // physical reads per defective sector (for Defect::pattern)
    QRandomGenerator m_rng;
    SimulatedStatistics m_stats;

    bool m_c2Enabled = false;
    bool m_probeMode = false;
    int m_speed = 0;
    int m_head = 0;

    struct CacheSegment {
        int first = 0;
        QByteArray data;
        QList<Rip::SectorStatus> status;
    };
    QList<CacheSegment> m_cache; // most recently used first
    QByteArray m_rwOnDisc; // the CD+G packs of the disc, interleaved as on the disc
};

// Reference result: the bytes a correct extraction of [firstLba, lastLba] must
// produce with the given correction offset (silence where the drive cannot
// deliver data).
QByteArray expectedAudio(const SimulatedSectorReader &reader, int firstLba, int lastLba, int correctionOffset, bool overread, Rip::ReadableArea area);

}
