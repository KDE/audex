/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QtGlobal>

namespace Audex::Rip
{

// Read offset correction (AccurateRip convention):
// A drive with correction +N delivers every sample N samples too early, so the
// correct sample i of the disc is found at drive sample position i + N.
//
// The plan maps an LBA range of the disc to the drive sectors that have to be
// read and to the number of bytes to discard before the corrected stream starts.
struct ReadPlan {
    int firstSector = 0; // drive sectors to read, inclusive
    int lastSector = -1;
    int skipBytes = 0; // bytes to discard at the start of firstSector
    qint64 outputBytes = 0; // corrected bytes to emit

    int sectorCount() const
    {
        return lastSector - firstSector + 1;
    }
};

qint64 floorDiv(qint64 a, qint64 b); // b > 0

ReadPlan makeReadPlan(int firstLba, int lastLba, int readOffsetSamples);

// The disc LBA whose corrected samples are (mostly) carried by a drive sector.
// Used to report error positions in disc time.
int correctedLbaForDriveSector(int driveSector, int readOffsetSamples);

}
