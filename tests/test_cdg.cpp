/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// CD+G: R-W sub-channel packs, their interleave on the disc and the
// extraction through the simulated drive.

#include "test_framework.h"

#include "core/cdg.h"
#include "core/subchannel.h"
#include "sim/simulatedsectorreader.h"

#include <QRandomGenerator>

#include <algorithm>

using namespace Audex;
using namespace Audex::Sim;

namespace
{

// graphics packs (tile blocks, color tables...) with a few empty ones between
QByteArray karaoke(int sectors, quint32 seed)
{
    static const char instructions[] = {1, 2, 6, 20, 24, 28, 30, 31, 38};
    QRandomGenerator rng(seed);
    QByteArray packs(qsizetype(sectors) * Cdda::SubcodeBytes, '\0');
    for (qsizetype p = 0; p < packs.size() / Cdda::CdgPackBytes; ++p) {
        if (rng.bounded(5) == 0)
            continue; // empty pack
        char *pack = packs.data() + p * Cdda::CdgPackBytes;
        pack[0] = 0x09;
        pack[1] = instructions[rng.bounded(int(sizeof(instructions)))];
        for (int i = 2; i < Cdda::CdgPackBytes; ++i)
            pack[i] = char(rng.bounded(64));
    }
    return packs;
}

SimulatedDisc cdgDisc()
{
    SimulatedDisc disc = SimulatedDisc::generate({600, 500, 400}, 0, 11);
    disc.cdg = karaoke(disc.toc.audioEndLba(), 5);
    return disc;
}

QByteArray slice(const QByteArray &packs, int firstLba, int lastLba)
{
    return packs.mid(qsizetype(firstLba) * Cdda::SubcodeBytes, qsizetype(lastLba - firstLba + 1) * Cdda::SubcodeBytes);
}

}

AUDEX_TEST("CD+G: interleave and de-interleave are inverse, the scramble included")
{
    const QByteArray packs = karaoke(20, 3);
    const QByteArray onDisc = Cdda::interleaveCdg(packs);
    AUDEX_CHECK(t, onDisc != packs);
    // symbol 18 of pack 0 is swapped to position 1, which is delayed by one pack
    AUDEX_EQUAL(t, int(onDisc.at(1 * Cdda::CdgPackBytes + 1)), int(packs.at(18)));
    // the last 7 packs lack the symbols of later packs
    const QByteArray back = Cdda::deinterleaveCdg(onDisc);
    const qsizetype full = packs.size() - 7 * Cdda::CdgPackBytes;
    AUDEX_EQUAL_DATA(t, back.first(full), packs.first(full));
    AUDEX_EQUAL(t, Cdda::cdgGraphicsPacks(back.first(full)), Cdda::cdgGraphicsPacks(packs.first(full)));
    AUDEX_CHECK(t, Cdda::cdgGraphicsPacks(onDisc) < Cdda::cdgGraphicsPacks(packs) / 4);
}

AUDEX_TEST("CD+G: detection")
{
    SimulatedSectorReader withCdg(cdgDisc(), DriveModel{});
    AUDEX_CHECK(t, Cdda::detectCdg(withCdg, withCdg.disc().toc));

    const auto plain = SimulatedDisc::generate({600, 500}, 0, 12);
    SimulatedSectorReader withoutCdg(plain, DriveModel{});
    AUDEX_CHECK(t, !Cdda::detectCdg(withoutCdg, plain.toc));

    DriveModel noRw;
    noRw.rwSubchannel = false;
    SimulatedSectorReader cannot(cdgDisc(), noRw);
    AUDEX_CHECK(t, !Cdda::detectCdg(cannot, cannot.disc().toc));
}

AUDEX_TEST("CD+G: extraction corrects the shift and de-interleaves when the drive does not")
{
    const SimulatedDisc disc = cdgDisc();
    for (const int shift : {0, 2, -3}) {
        for (const bool deinterleaved : {false, true}) {
            DriveModel model;
            model.subchannelShift = shift;
            model.rwDeinterleaved = deinterleaved;
            SimulatedSectorReader reader(disc, model);
            const Cdda::Track *track = disc.toc.track(2);
            const Cdda::CdgExtraction x = Cdda::extractCdg(reader, disc.toc, track->firstLba, track->lastLba);

            AUDEX_CHECK_MSG(t, x.error.isEmpty(), x.error);
            AUDEX_CHECK(t, x.found);
            AUDEX_CHECK(t, x.shiftMeasured);
            AUDEX_EQUAL(t, x.shift, shift);
            AUDEX_CHECK(t, x.deinterleavedByDrive == deinterleaved);
            AUDEX_EQUAL_DATA(t, x.packs, slice(disc.cdg, track->firstLba, track->lastLba));
        }
    }
}

AUDEX_TEST("CD+G: extraction of the whole disc, at its end and with read errors")
{
    const SimulatedDisc disc = cdgDisc();
    DriveModel model;
    model.rwErrorRate = 0.02;
    model.subchannelShift = 1;
    SimulatedSectorReader reader(disc, model);
    const int last = disc.toc.audioEndLba() - 1;
    const Cdda::CdgExtraction x = Cdda::extractCdg(reader, disc.toc, 0, last);

    AUDEX_CHECK(t, x.found);
    AUDEX_CHECK_MSG(t, x.rereadSectors > 0 && x.unresolvedSectors == 0, QString::number(x.rereadSectors) + u'/' + QString::number(x.unresolvedSectors));
    // Sector 0 comes with LBA -1 on this drive, which is not read (lead-in);
    // the last 7 packs need data behind the disc. Compare in between.
    const QByteArray expected = slice(disc.cdg, 1, last - 2);
    AUDEX_EQUAL_DATA(t, x.packs.mid(Cdda::SubcodeBytes, expected.size()), expected);
}

AUDEX_TEST("CD+G: a disc without graphics and a drive without the raw sub-channel")
{
    const auto plain = SimulatedDisc::generate({600}, 0, 13);
    SimulatedSectorReader reader(plain, DriveModel{});
    const Cdda::CdgExtraction none = Cdda::extractCdg(reader, plain.toc, 0, 599);
    AUDEX_CHECK(t, !none.found && none.error.isEmpty() && none.graphicsPacks == 0);

    DriveModel noRw;
    noRw.rwSubchannel = false;
    SimulatedSectorReader cannot(plain, noRw);
    const Cdda::CdgExtraction failed = Cdda::extractCdg(cannot, plain.toc, 0, 599);
    AUDEX_CHECK(t, !failed.found && !failed.error.isEmpty());
}

AUDEX_TEST("CD+G: progress through both reads and the sectors read again")
{
    const SimulatedDisc disc = cdgDisc();
    DriveModel model;
    model.rwErrorRate = 0.02; // some sectors differ between the two reads
    SimulatedSectorReader reader(disc, model);
    const int last = disc.toc.audioEndLba() - 1;

    QList<Cdda::CdgProgress> reports;
    QStringList messages;
    const Cdda::CdgExtraction x = Cdda::extractCdg(
        reader,
        disc.toc,
        0,
        last,
        [&](const QString &text) {
            messages << text;
        },
        {},
        [&](const Cdda::CdgProgress &p) {
            reports << p;
        });
    AUDEX_CHECK(t, x.found && x.rereadSectors > 0);
    AUDEX_CHECK(t, reports.size() > 10);
    if (reports.isEmpty())
        return;

    // two reads of the whole disc, then the differing sectors
    const qint64 sectors = disc.toc.audioEndLba();
    AUDEX_EQUAL(t, reports.first().pass, 1);
    AUDEX_EQUAL(t, reports.first().done, qint64(0));
    AUDEX_EQUAL(t, reports.last().pass, 3);
    AUDEX_EQUAL(t, reports.last().total, 2 * sectors + x.rereadSectors);
    AUDEX_EQUAL(t, reports.last().done, reports.last().total); // ends at 100 %

    bool ordered = true;
    bool inside = true;
    for (qsizetype i = 0; i < reports.size(); ++i) {
        const Cdda::CdgProgress &p = reports.at(i);
        inside = inside && p.lba >= 0 && p.lba <= sectors && p.done <= p.total;
        if (i > 0)
            ordered = ordered && p.done >= reports.at(i - 1).done && p.pass >= reports.at(i - 1).pass;
    }
    AUDEX_CHECK(t, ordered);
    AUDEX_CHECK(t, inside);
    // the second read starts where the first one ended
    const auto second = std::find_if(reports.cbegin(), reports.cend(), [](const Cdda::CdgProgress &p) {
        return p.pass == 2;
    });
    AUDEX_CHECK(t, second != reports.cend() && second->done == sectors && second->lba == 0);
    AUDEX_EQUAL(t, messages.size(), qsizetype(3)); // 1 of 2, 2 of 2, read again
}
