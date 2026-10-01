/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include "core/cdda.h"

namespace Audex::Rip
{

struct SectorStatus {
    bool ok = false; // data is usable (possibly after drive-internal recovery)
    bool recovered = false; // drive reported RECOVERED ERROR
    quint16 c2Errors = 0; // number of bytes flagged by C2 pointers (if enabled)
};

struct SectorReadResult {
    QByteArray audio; // always count * SectorBytes; failed sectors are zero-filled
    QList<SectorStatus> sectors;
    QString error; // description of the first failure, if any
    qint64 elapsedUs = 0; // time the drive needed (for cache detection)
    bool splitIntoSingleReads = false; // the burst failed and was read sector by sector

    bool allOk() const
    {
        for (const SectorStatus &s : sectors)
            if (!s.ok)
                return false;
        return true;
    }

    QByteArrayView sector(int index) const
    {
        return QByteArrayView(audio).sliced(qsizetype(index) * Cdda::SectorBytes, Cdda::SectorBytes);
    }
};

// Abstraction of "a thing that returns raw CD-DA sectors". Implemented by the
// real MMC drive and by the simulated drive used in tests.
//
// A read never throws and never returns short data: failed sectors are marked
// in `sectors` and zero-filled in `audio`. Retrying is the caller's business.
class SectorReader
{
public:
    virtual ~SectorReader() = default;

    virtual SectorReadResult read(int lba, int count) = 0;

    virtual int maxSectorsPerRead() const = 0;

    virtual bool supportsC2() const
    {
        return false;
    }
    virtual void setC2Enabled(bool enabled)
    {
        Q_UNUSED(enabled)
    }

    // Formatted Q sub-channel, 12 bytes per sector (see subchannel.h);
    // empty if the reader cannot deliver it.
    virtual QByteArray readSubchannelQ(int lba, int count)
    {
        Q_UNUSED(lba)
        Q_UNUSED(count)
        return {};
    }

    // Raw P-W sub-channel, 96 bytes per sector as the drive delivers it (Q in
    // bit 6, R-W in bits 5..0; see cdg.h); empty if the reader cannot
    // deliver it, shorter if a command failed.
    virtual QByteArray readSubchannelRaw(int lba, int count)
    {
        Q_UNUSED(lba)
        Q_UNUSED(count)
        return {};
    }

    // factor in "x" (1x = 75 sectors/s), 0 = maximum. Returns false if unsupported.
    virtual bool setReadSpeed(int factor)
    {
        Q_UNUSED(factor)
        return false;
    }

    // Probing commands a drive or its USB bridge may not answer (drive
    // assistant): a short timeout, and a failed read command is not retried
    // sector by sector.
    virtual void setProbeMode(bool enabled)
    {
        Q_UNUSED(enabled)
    }

    // Commands the drive did not answer in time, since the reader was created.
    // Such a command is aborted by the kernel, often with a reset of the drive.
    virtual int commandTimeouts() const
    {
        return 0;
    }

    virtual QString description() const = 0;
};

}
