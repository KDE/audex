/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "core/ripengine.h"
#include "core/sectorreader.h"

#include <QStringList>

namespace Audex::Rip
{

struct CacheDefeatCalibration {
    bool cachingDetected = false; // an immediate re-read is answered from the cache
    bool defeated = true; // false: even maxReads far reads did not reach the disc
    int reads = 1; // value for RipOptions::cacheDefeatReads
    bool timedOut = false; // the drive did not answer a read in time: measurement stopped
    QStringList details;
};

// Timing based: whether immediate re-reads come from a cache (measured at three
// positions after a few warm-up reads, majority decides), and how many far
// reads at different positions (as the engine does them) are needed before a
// re-read reaches the disc again. Takes a few seconds on a real drive, so the
// result should be stored per drive.
CacheDefeatCalibration calibrateCacheDefeat(SectorReader &reader, ReadableArea area, int distance, int sectors = 1, int maxReads = 16, int repetitions = 5);

}
