/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "core/cacheprobe.h"
#include "sim/simulatedsectorreader.h"

using namespace Audex;
using namespace Audex::Rip;
using namespace Audex::Sim;

namespace
{

CacheDefeatCalibration calibrate(const DriveModel &model)
{
    const auto disc = SimulatedDisc::generate({6000, 6000}, 0, 5);
    SimulatedSectorReader reader(disc, model);
    return calibrateCacheDefeat(reader, ReadableArea{disc.toc.audioStartLba(), disc.toc.audioEndLba()}, 1000);
}

}

AUDEX_TEST("cache probe: an audio cache is found and defeated")
{
    DriveModel model;
    model.cacheSectors = 256;
    const CacheDefeatCalibration cal = calibrate(model);
    AUDEX_CHECK_MSG(t, cal.cachingDetected, cal.details.join(u'\n'));
    AUDEX_CHECK(t, cal.defeated);
    AUDEX_EQUAL(t, cal.reads, 1);
}

AUDEX_TEST("cache probe: a drive without cache")
{
    const CacheDefeatCalibration cal = calibrate(DriveModel{});
    AUDEX_CHECK_MSG(t, !cal.cachingDetected, cal.details.join(u'\n'));
}

AUDEX_TEST("cache probe: a slow phase of the drive does not hide the cache")
{
    // e.g. a speed change: five commands take 440 ms longer, at different
    // moments of the measurement (the first one covers the re-reads that a
    // single measurement without warm-up took as its only sample)
    for (int from : {2, 6, 13}) {
        DriveModel model;
        model.cacheSectors = 256;
        model.slowFrom = from;
        model.slowCommands = 5;
        model.slowUs = 440000;
        const CacheDefeatCalibration cal = calibrate(model);
        AUDEX_CHECK_MSG(t, cal.cachingDetected, cal.details.join(u'\n'));
    }
}

AUDEX_TEST("cache probe: a slow first read does not fake a cache")
{
    // the first read at the first position takes long, so its re-reads look cached
    DriveModel model;
    model.slowFrom = 5;
    model.slowCommands = 1;
    model.slowUs = 440000;
    const CacheDefeatCalibration cal = calibrate(model);
    AUDEX_CHECK_MSG(t, !cal.cachingDetected, cal.details.join(u'\n'));
    AUDEX_CHECK_MSG(t, cal.details.join(u'\n').contains(QStringLiteral("(cached)")), cal.details.join(u'\n'));
}
