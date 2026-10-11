/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "core/ripreport.h"

using namespace Audex;
using namespace Qt::StringLiterals;

AUDEX_TEST("rip log: how each track was read")
{
    Rip::RipOptions secure;
    secure.mode = Rip::ReadMode::Secure;
    Rip::SegmentResult clean;
    Rip::SegmentResult differed;
    differed.passMismatches = 3;
    Rip::SegmentResult confirmed;
    confirmed.acceptedAfterOnePass = true;

    AUDEX_EQUAL(t, Rip::readDescription(secure, clean), u"Read twice and compared: no differences"_s);
    AUDEX_EQUAL(t, Rip::readDescription(secure, differed), u"Read twice and compared: 3 sector(s) differed"_s);
    AUDEX_EQUAL(t, Rip::readDescription(secure, confirmed, u"the CUETools database"_s), u"Read once, confirmed by the CUETools database"_s);

    Rip::RipOptions c2 = secure;
    c2.useC2 = true;
    AUDEX_EQUAL(t, Rip::readDescription(c2, clean), u"Read once, checked with C2 error pointers"_s);

    Rip::RipOptions fast;
    fast.mode = Rip::ReadMode::Fast;
    AUDEX_EQUAL(t, Rip::readDescription(fast, clean), u"Read once"_s);
}

AUDEX_TEST("rip log: the read line stands before the copy CRC")
{
    Rip::ReportContext ctx;
    ctx.options.mode = Rip::ReadMode::Secure;
    ctx.fileNames = {u"01.flac"_s, u"02.flac"_s};
    ctx.confirmedBy.insert(1, u"AccurateRip"_s);

    Rip::RipResult result;
    Rip::SegmentResult first;
    first.segment.trackNumber = 1;
    Rip::SegmentResult second;
    second.segment.trackNumber = 2;
    second.acceptedAfterOnePass = true;
    result.segments = {first, second};

    const QStringList log = Rip::formatReport(ctx, result);
    const qsizetype read = log.indexOf(u"     Read twice and compared: no differences"_s);
    AUDEX_CHECK(t, read > 0 && log.value(read + 1).startsWith(u"     Copy CRC"_s));
    AUDEX_CHECK(t, log.contains(u"     Read once, confirmed by AccurateRip"_s));
}
