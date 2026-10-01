/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "core/subchannel.h"
#include "sim/simulatedsectorreader.h"

using namespace Audex;
using namespace Audex::Sim;
using namespace Qt::StringLiterals;

namespace
{

SimulatedDisc discWithIndexes()
{
    SimulatedDisc disc = SimulatedDisc::generate({600, 500, 400}, 0, 7);
    disc.index00.insert(2, disc.toc.track(2)->firstLba - 75);
    disc.indexes.insert(3, {disc.toc.track(3)->firstLba + 120});
    disc.isrc.insert(1, u"DEABC2600001"_s);
    return disc;
}

}

AUDEX_TEST("Q sub-channel scan finds pregaps and indexes of a shifted drive")
{
    const SimulatedDisc disc = discWithIndexes();
    for (int shift : {0, 1, 3, -2}) {
        DriveModel model;
        model.subchannelShift = shift;
        SimulatedSectorReader reader(disc, model);
        const Cdda::SubchannelScan scan = Cdda::scanSubchannel(reader, disc.toc, true);

        AUDEX_CHECK(t, scan.supported);
        AUDEX_EQUAL(t, scan.index00.value(2, -1), disc.index00.value(2));
        AUDEX_EQUAL(t, scan.indexes.value(3).value(0, -1), disc.indexes.value(3).at(0));
        AUDEX_CHECK(t, scan.isrc.value(1) == disc.isrc.value(1));
    }
}

AUDEX_TEST("Q sub-channel: a short read skips only the sector that failed")
{
    // the ISRC frames of track 1 are at LBA 50 and 150; the reads that
    // should deliver them stop at LBA 40 and 140
    const SimulatedDisc disc = discWithIndexes();
    DriveModel model;
    model.subchannelUnreadable = {40, 140};
    SimulatedSectorReader reader(disc, model);
    const Cdda::SubchannelScan scan = Cdda::scanSubchannel(reader, disc.toc, true);
    AUDEX_CHECK(t, scan.supported);
    AUDEX_CHECK_MSG(t, scan.isrc.value(1) == disc.isrc.value(1), scan.isrc.value(1));
}
