/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "core/drivefeatures.h"
#include "sim/simulatedsectorreader.h"

using namespace Audex;
using namespace Audex::Rip;
using namespace Audex::Sim;

namespace
{

struct C2Run {
    DriveFeatures features;
    FeatureCheck check; // the C2 check
    qint64 commands = 0; // read commands of the C2 check
};

C2Run runC2(const DriveModel &model, bool cancelAtC2 = false)
{
    const auto disc = SimulatedDisc::generate({3000, 3000}, 0, 31);
    SimulatedSectorReader reader(disc, model);
    C2Run run;
    qint64 before = 0;
    bool canceled = false;
    FeatureCallbacks cb;
    cb.started = [&](int, int, const FeatureCheck &check) {
        if (check.id == QStringLiteral("c2")) {
            before = reader.statistics().commands;
            canceled = cancelAtC2;
        }
    };
    cb.finished = [&](const FeatureCheck &check) {
        if (check.id == QStringLiteral("c2")) {
            run.check = check;
            run.commands = reader.statistics().commands - before;
        }
    };
    cb.isCanceled = [&canceled] {
        return canceled;
    };
    run.features = detectDriveFeatures(reader, disc.toc, cb);
    return run;
}

struct FullRun {
    DriveFeatures features;
    qint64 timeouts = 0;
    qint64 timeoutWaitMs = 0;
};

FullRun runAll(const DriveModel &model)
{
    const auto disc = SimulatedDisc::generate({3000, 3000}, 0, 31);
    SimulatedSectorReader reader(disc, model);
    FullRun run;
    run.features = detectDriveFeatures(reader, disc.toc);
    run.timeouts = reader.statistics().timeouts;
    run.timeoutWaitMs = reader.statistics().timeoutWaitMs;
    return run;
}

const FeatureCheck *findCheck(const DriveFeatures &features, const QString &id)
{
    for (const FeatureCheck &c : features.checks)
        if (c.id == id)
            return &c;
    return nullptr;
}

}

AUDEX_TEST("drive features: C2 pointers that cannot be read end the check at once")
{
    DriveModel model;
    model.c2Capable = true;
    model.c2ReadsFail = true;
    const C2Run run = runC2(model);
    AUDEX_CHECK(t, !run.features.c2Supported);
    AUDEX_CHECK(t, run.features.c2Reliable == Feature::No);
    AUDEX_CHECK(t, run.check.verdict == Feature::No);
    AUDEX_CHECK_MSG(t, run.check.summary.contains(QStringLiteral("fail")), run.check.summary);
    // finding the position (without C2) and a single probe, no repeated reads
    AUDEX_CHECK_MSG(t, run.commands <= 7, QString::number(run.commands));
    // the checks after it still run
    AUDEX_EQUAL(t, run.features.checks.size(), qsizetype(7));
}

AUDEX_TEST("drive features: C2 check of a working drive")
{
    DriveModel model;
    model.c2Capable = true;
    const C2Run run = runC2(model);
    AUDEX_CHECK(t, run.features.c2Supported);
    AUDEX_CHECK_MSG(t, run.check.details.join(u'\n').contains(QStringLiteral("6 reads")), run.check.details.join(u'\n'));
    AUDEX_CHECK_MSG(t, run.check.summary.contains(QStringLiteral("no read errors")), run.check.summary);
}

AUDEX_TEST("drive features: stopping during the C2 check reads nothing more")
{
    DriveModel model;
    model.c2Capable = true;
    const C2Run run = runC2(model, true);
    AUDEX_EQUAL(t, run.commands, 0LL);
    AUDEX_CHECK(t, run.features.canceled);
}

AUDEX_TEST("drive features: commands the drive does not answer cost one timeout each")
{
    // times out on C2 reads of several sectors, outside the audio area and on
    // Q sub-channel reads, but answers plain reads afterwards
    DriveModel model;
    model.c2Capable = true;
    model.c2BlockReadsTimeOut = true;
    model.overreadTimesOut = true;
    model.subchannelTimesOut = true;
    const FullRun run = runAll(model);

    AUDEX_CHECK(t, run.features.measured.isValid());
    AUDEX_CHECK(t, !run.features.driveStalled);
    AUDEX_EQUAL(t, run.features.checks.size(), qsizetype(7));
    // one C2 block read, lead-in, lead-out, one Q read, each with the short
    // timeout of a probe
    AUDEX_EQUAL(t, run.timeouts, 4LL);
    AUDEX_EQUAL(t, run.timeoutWaitMs, 4 * 8000LL);

    AUDEX_CHECK(t, !run.features.c2Supported);
    const FeatureCheck *c2 = findCheck(run.features, QStringLiteral("c2"));
    AUDEX_CHECK_MSG(t, c2 && c2->summary.contains(QStringLiteral("no answer")), c2 ? c2->summary : QString());
    AUDEX_CHECK(t, run.features.leadIn == Feature::No && run.features.leadOut == Feature::No);
    AUDEX_CHECK(t, run.features.subchannelQ == Feature::No);
    const FeatureCheck *q = findCheck(run.features, QStringLiteral("subchannel"));
    AUDEX_CHECK_MSG(t, q && q->summary.contains(QStringLiteral("does not answer")), q ? q->summary : QString());
}

AUDEX_TEST("drive features: a drive that stops answering ends the test")
{
    DriveModel model;
    model.overreadTimesOut = true;
    model.stallsAfterTimeouts = 1;
    const FullRun run = runAll(model);

    AUDEX_CHECK(t, run.features.driveStalled);
    AUDEX_CHECK(t, run.features.measured.isValid()); // what was measured can be stored
    AUDEX_EQUAL(t, run.features.checks.size(), qsizetype(7));
    // the lead-in read, and the plain read that shows the drive no longer answers
    AUDEX_EQUAL(t, run.timeouts, 2LL);
    AUDEX_CHECK(t, run.features.leadOut == Feature::Unknown);
    AUDEX_CHECK_MSG(t, run.features.stalledAfter == QStringLiteral("overread"), run.features.stalledAfter);
    AUDEX_CHECK(t, run.features.subchannelQ == Feature::Yes); // measured before
    const FeatureCheck *c2 = findCheck(run.features, QStringLiteral("c2"));
    AUDEX_CHECK_MSG(t, c2 && c2->summary.contains(QStringLiteral("stopped answering")), c2 ? c2->summary : QString());
}

AUDEX_TEST("drive features: the checks that can hang a drive come last")
{
    // C2 reads of a block hang this drive until it is switched off
    const auto disc = SimulatedDisc::generate({3000, 3000}, 0, 31);
    DriveModel model;
    model.c2Capable = true;
    model.c2BlockReadsTimeOut = true;
    model.stallsAfterTimeouts = 1;
    SimulatedSectorReader reader(disc, model);
    bool offsetRan = false;
    FeatureCallbacks cb;
    cb.offset = [&] {
        offsetRan = true;
        FeatureCheck check;
        check.verdict = Feature::Yes;
        check.summary = QStringLiteral("6 samples");
        return check;
    };
    const DriveFeatures features = detectDriveFeatures(reader, disc.toc, cb);

    QStringList ids;
    for (const FeatureCheck &c : features.checks)
        ids << c.id;
    AUDEX_CHECK_MSG(t, ids.join(u',') == QStringLiteral("burst,cache,stream,subchannel,offset,rw,overread,c2"), ids.join(u','));
    AUDEX_CHECK(t, offsetRan);
    // everything but C2 was measured before the drive stopped answering
    AUDEX_CHECK(t, features.subchannelQ == Feature::Yes);
    AUDEX_CHECK(t, features.leadIn == Feature::No && features.leadOut == Feature::No);
    AUDEX_CHECK(t, !features.c2Supported);
    // the caller learns that the drive is gone and must not send it anything
    AUDEX_CHECK(t, features.driveStalled);
    AUDEX_CHECK_MSG(t, features.stalledAfter == QStringLiteral("c2"), features.stalledAfter);
    // the C2 read and the probe read after it, both with the short timeout
    AUDEX_EQUAL(t, reader.statistics().timeouts, 2LL);
    AUDEX_EQUAL(t, reader.statistics().timeoutWaitMs, 2 * 8000LL);
}

AUDEX_TEST("drive features: a read size the drive cannot do is not retried sector by sector")
{
    const auto disc = SimulatedDisc::generate({3000, 3000}, 0, 31);
    DriveModel model;
    model.maxSectorsPerRead = 10; // e.g. a USB bridge
    SimulatedSectorReader reader(disc, model);
    qint64 commands = 0;
    FeatureCallbacks cb;
    cb.finished = [&](const FeatureCheck &check) {
        if (check.id == QStringLiteral("burst"))
            commands = reader.statistics().commands;
    };
    const DriveFeatures features = detectDriveFeatures(reader, disc.toc, cb);
    AUDEX_EQUAL(t, features.burstSectors, 10);
    // 57 commands (positions, cache defeat, blocks, comparisons); splitting
    // the failed blocks into single reads added about 150 more
    AUDEX_CHECK_MSG(t, commands < 80, QString::number(commands));
}

AUDEX_TEST("drive features: the raw sub-channel and its shift")
{
    DriveModel model;
    model.subchannelShift = 2;
    FullRun run = runAll(model);
    AUDEX_CHECK(t, run.features.rwSubchannel == Feature::Yes);
    AUDEX_EQUAL(t, run.features.rwShift, 2);
    const FeatureCheck *rw = findCheck(run.features, QStringLiteral("rw"));
    AUDEX_CHECK_MSG(t, rw && rw->summary.contains(QStringLiteral("2 sector(s) off")), rw ? rw->summary : QString());

    model = DriveModel();
    model.rwSubchannel = false;
    run = runAll(model);
    AUDEX_CHECK(t, run.features.rwSubchannel == Feature::No);
}

AUDEX_TEST("drive features: a drive without accurate stream is found")
{
    DriveModel model;
    model.jitterSamples = 12;
    const FullRun run = runAll(model);
    AUDEX_CHECK(t, run.features.accurateStream == Feature::No);
    // the shift between two reads: each of them up to 12 samples off
    AUDEX_CHECK(t, run.features.jitterSamples > 0 && run.features.jitterSamples <= 24);
    const FeatureCheck *stream = findCheck(run.features, QStringLiteral("stream"));
    AUDEX_CHECK_MSG(t, stream && stream->summary.contains(QStringLiteral("cannot compensate")), stream ? stream->summary : QString());

    const FullRun accurate = runAll(DriveModel());
    AUDEX_CHECK(t, accurate.features.accurateStream == Feature::Yes);
    AUDEX_EQUAL(t, accurate.features.jitterSamples, 0);
}
