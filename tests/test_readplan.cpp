/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Tests for the read plan (LBA range -> drive sectors + skip bytes),
// correctedLbaForDriveSector() and the cache defeat positioning.

#include "test_framework.h"

#include "core/readplan.h"
#include "core/ripengine.h"

using namespace Audex::Rip;
using Audex::Tests::Context;

AUDEX_TEST("floorDiv rounds towards negative infinity")
{
    AUDEX_EQUAL(t, floorDiv(0LL, 588LL), 0LL);
    AUDEX_EQUAL(t, floorDiv(587LL, 588LL), 0LL);
    AUDEX_EQUAL(t, floorDiv(1176LL, 588LL), 2LL);
    AUDEX_EQUAL(t, floorDiv(1177LL, 588LL), 2LL);
    AUDEX_EQUAL(t, floorDiv(-1LL, 588LL), -1LL);
    AUDEX_EQUAL(t, floorDiv(-587LL, 588LL), -1LL);
    AUDEX_EQUAL(t, floorDiv(-588LL, 588LL), -1LL);
    AUDEX_EQUAL(t, floorDiv(-1177LL, 588LL), -3LL);
}

AUDEX_TEST("read plan without offset")
{
    const ReadPlan plan = makeReadPlan(10, 12, 0);
    AUDEX_EQUAL(t, plan.firstSector, 10);
    AUDEX_EQUAL(t, plan.lastSector, 12);
    AUDEX_EQUAL(t, plan.skipBytes, 0);
    AUDEX_EQUAL(t, plan.outputBytes, 3LL * 2352);
    AUDEX_EQUAL(t, plan.sectorCount(), 3);
}

AUDEX_TEST("read plan of an empty range is empty")
{
    const ReadPlan plan = makeReadPlan(10, 9, 137);
    AUDEX_EQUAL(t, plan.firstSector, 0);
    AUDEX_EQUAL(t, plan.lastSector, -1);
    AUDEX_EQUAL(t, plan.skipBytes, 0);
    AUDEX_EQUAL(t, plan.outputBytes, 0LL);
    AUDEX_EQUAL(t, plan.sectorCount(), 0);
}

AUDEX_TEST("read plan with positive offset")
{
    // +1 sample: the corrected stream starts 4 bytes into drive sector 10 and
    // reaches 1 sample into drive sector 11
    const ReadPlan sub = makeReadPlan(10, 10, 1);
    AUDEX_EQUAL(t, sub.firstSector, 10);
    AUDEX_EQUAL(t, sub.lastSector, 11);
    AUDEX_EQUAL(t, sub.skipBytes, 4);
    AUDEX_EQUAL(t, sub.outputBytes, 2352LL);

    // +587 samples: still two drive sectors, almost a whole sector to skip
    const ReadPlan edge = makeReadPlan(10, 10, 587);
    AUDEX_EQUAL(t, edge.firstSector, 10);
    AUDEX_EQUAL(t, edge.lastSector, 11);
    AUDEX_EQUAL(t, edge.skipBytes, 587 * 4);
    AUDEX_EQUAL(t, edge.outputBytes, 2352LL);

    // +588 samples (a whole sector): the plan shifts by one sector, no skip
    const ReadPlan sector = makeReadPlan(10, 10, 588);
    AUDEX_EQUAL(t, sector.firstSector, 11);
    AUDEX_EQUAL(t, sector.lastSector, 11);
    AUDEX_EQUAL(t, sector.skipBytes, 0);
    AUDEX_EQUAL(t, sector.outputBytes, 2352LL);
}

AUDEX_TEST("read plan with negative offset")
{
    // -1 sample: the corrected stream starts 4 bytes before the end of drive
    // sector 9
    const ReadPlan sub = makeReadPlan(10, 10, -1);
    AUDEX_EQUAL(t, sub.firstSector, 9);
    AUDEX_EQUAL(t, sub.lastSector, 10);
    AUDEX_EQUAL(t, sub.skipBytes, 587 * 4);
    AUDEX_EQUAL(t, sub.outputBytes, 2352LL);

    // -588 samples (a whole sector): the plan shifts by one sector, no skip
    const ReadPlan sector = makeReadPlan(10, 10, -588);
    AUDEX_EQUAL(t, sector.firstSector, 9);
    AUDEX_EQUAL(t, sector.lastSector, 9);
    AUDEX_EQUAL(t, sector.skipBytes, 0);
    AUDEX_EQUAL(t, sector.outputBytes, 2352LL);

    // -137 samples (a typical drive offset), three sector range
    const ReadPlan typical = makeReadPlan(10, 12, -137);
    AUDEX_EQUAL(t, typical.firstSector, 9);
    AUDEX_EQUAL(t, typical.lastSector, 12);
    AUDEX_EQUAL(t, typical.skipBytes, 451 * 4);
    AUDEX_EQUAL(t, typical.outputBytes, 3LL * 2352);

    // -600 samples: more than one sector back, still not sector aligned
    const ReadPlan overSector = makeReadPlan(10, 12, -600);
    AUDEX_EQUAL(t, overSector.firstSector, 8);
    AUDEX_EQUAL(t, overSector.lastSector, 11);
    AUDEX_EQUAL(t, overSector.skipBytes, 576 * 4);
    AUDEX_EQUAL(t, overSector.outputBytes, 3LL * 2352);
}

AUDEX_TEST("corrected LBA for drive sector")
{
    // The centre sample of the drive sector decides which disc sector the
    // drive sector mostly carries (see readplan.h).
    AUDEX_EQUAL(t, correctedLbaForDriveSector(100, 0), 100);
    AUDEX_EQUAL(t, correctedLbaForDriveSector(0, 0), 0);
    AUDEX_EQUAL(t, correctedLbaForDriveSector(-1, 0), -1);

    // Small offsets do not move the centre sample into the neighbour sector
    AUDEX_EQUAL(t, correctedLbaForDriveSector(100, 1), 100);
    AUDEX_EQUAL(t, correctedLbaForDriveSector(100, -1), 100);

    // ... but an offset larger than half a sector does
    AUDEX_EQUAL(t, correctedLbaForDriveSector(100, 294), 100);
    AUDEX_EQUAL(t, correctedLbaForDriveSector(100, 295), 99);
    AUDEX_EQUAL(t, correctedLbaForDriveSector(100, -293), 100);
    AUDEX_EQUAL(t, correctedLbaForDriveSector(100, -294), 101);

    // Whole sector offsets
    AUDEX_EQUAL(t, correctedLbaForDriveSector(100, 588), 99);
    AUDEX_EQUAL(t, correctedLbaForDriveSector(100, -588), 101);
}

AUDEX_TEST("cache defeat targets keep their distance")
{
    const ReadableArea area{0, 30000};
    const int lba = 9000;
    const int count = 512;
    const int distance = 5000;

    QList<int> targets;
    for (int i = 0; i < 4; ++i) {
        bool fits = false;
        const int target = cacheDefeatTarget(area, lba, count, i, 4, distance, 1, &fits);
        AUDEX_CHECK(t, fits);
        AUDEX_CHECK(t, !targets.contains(target));
        // at least `distance` away from the range that is to be re-read
        AUDEX_CHECK(t, target >= lba + count + distance || target <= lba - distance - 1);
        // inside the readable area
        AUDEX_CHECK(t, target >= area.firstLba && target < area.endLba);
        targets.append(target);
    }
    // the positions alternate between in front of and behind the range
    AUDEX_CHECK(t, targets.at(0) > lba + count);
    AUDEX_CHECK(t, targets.at(1) < lba);
}

AUDEX_TEST("cache defeat reads of several sectors stay inside the area")
{
    const ReadableArea area{0, 30000};
    for (int i = 0; i < 2; ++i) {
        bool fits = false;
        const int target = cacheDefeatTarget(area, 9000, 512, i, 2, 5000, 3, &fits);
        AUDEX_CHECK(t, fits);
        AUDEX_CHECK(t, target >= area.firstLba && target + 3 <= area.endLba);
        AUDEX_CHECK(t, target >= 9000 + 512 + 5000 || target + 3 <= 9000 - 5000);
    }
}

AUDEX_TEST("cache defeat targets on a short disc")
{
    // A 7000 sector audio area has no room for 4 defeat reads 5000 sectors
    // apart: the positions move closer together, but stay distinct and inside
    // the area, and the caller is told via fits = false.
    const ReadableArea area{0, 7000};
    QList<int> targets;
    for (int i = 0; i < 4; ++i) {
        bool fits = true;
        const int target = cacheDefeatTarget(area, 0, 512, i, 4, 5000, 1, &fits);
        AUDEX_CHECK(t, !fits);
        AUDEX_CHECK(t, !targets.contains(target));
        AUDEX_CHECK(t, target >= area.firstLba && target < area.endLba);
        targets.append(target);
    }
}

AUDEX_TEST("cache defeat target without any room falls back to the area edge")
{
    // Both sides of the range are smaller than the distance: the far read is
    // placed at the widest remaining spot instead of keeping the distance.
    const ReadableArea area{0, 100};
    bool fits = true;
    const int target = cacheDefeatTarget(area, 10, 5, 0, 1, 5000, 1, &fits);
    AUDEX_CHECK(t, !fits);
    AUDEX_EQUAL(t, target, 99);
}
