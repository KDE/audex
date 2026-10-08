/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "readplan.h"

#include "cdda.h"

namespace Audex::Rip
{

qint64 floorDiv(qint64 a, qint64 b)
{
    Q_ASSERT(b > 0);
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

ReadPlan makeReadPlan(int firstLba, int lastLba, int readOffsetSamples)
{
    using namespace Cdda;

    ReadPlan plan;
    if (lastLba < firstLba)
        return plan;

    // Sample range in drive space, end exclusive
    const qint64 firstSample = qint64(firstLba) * SamplesPerSector + readOffsetSamples;
    const qint64 endSample = qint64(lastLba + 1) * SamplesPerSector + readOffsetSamples;

    plan.firstSector = int(floorDiv(firstSample, SamplesPerSector));
    plan.lastSector = int(floorDiv(endSample - 1, SamplesPerSector));
    plan.skipBytes = int(firstSample - qint64(plan.firstSector) * SamplesPerSector) * BytesPerSample;
    plan.outputBytes = (endSample - firstSample) * BytesPerSample;
    return plan;
}

int correctedLbaForDriveSector(int driveSector, int readOffsetSamples)
{
    using namespace Cdda;
    // centre sample of the drive sector, mapped back to disc space
    const qint64 centre = qint64(driveSector) * SamplesPerSector + SamplesPerSector / 2 - readOffsetSamples;
    return int(floorDiv(centre, SamplesPerSector));
}

}
