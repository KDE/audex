/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "core/sectorreader.h"
#include "scsidevice.h"

namespace Audex::Mmc
{

class MmcSectorReader : public Rip::SectorReader
{
public:
    // maxTransferBytes: conservative default that also works with most USB bridges
    MmcSectorReader(Scsi::Device &device, bool c2Capable, QString description, int maxTransferBytes = 65536);

    Rip::SectorReadResult read(int lba, int count) override;
    int maxSectorsPerRead() const override;
    bool supportsC2() const override;
    void setC2Enabled(bool enabled) override;
    void setProbeMode(bool enabled) override;
    bool setReadSpeed(int factor) override;
    QByteArray readSubchannelQ(int lba, int count) override;
    QByteArray readSubchannelRaw(int lba, int count) override;
    int commandTimeouts() const override;
    QString description() const override;

private:
    Rip::SectorReadResult readCommand(int lba, int count, Scsi::Result *scsi);
    int bytesPerSector() const;

    const Scsi::Result &note(const Scsi::Result &result);

    int timeoutMs() const;

    Scsi::Device &m_device;
    bool m_c2Capable;
    bool m_c2Enabled = false;
    QString m_description;
    int m_maxTransferBytes;
    int m_timeouts = 0;
    bool m_probeMode = false;
};

}
