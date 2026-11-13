/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "core/discid.h"
#include "core/ripreport.h"
#include "core/toc.h"

#include <QRegularExpression>

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

// MusicBrainz Picard reads the TOC table of EAC, XLD and fre:ac logs with two
// expressions (picard/disc/eaclog.py): a header of five columns separated by
// "|", one line to skip, then rows until the first that does not fit. The
// disc ID comes from start sector + 150 and the last end sector + 151.
AUDEX_TEST("rip log: the TOC table is the one Picard reads")
{
    Cdda::Toc toc;
    toc.tracks = {Cdda::Track{1, 1, 150, 20366}, Cdda::Track{2, 1, 20367, 38129}, Cdda::Track{3, 1, 38130, 60000}};
    toc.leadOutLba = 60001;
    toc.source = u"simulation"_s;
    const QStringList lines = toc.describe();

    static const QRegularExpression header(u"^\\s*\\s*.+\\s+\\|\\s+.+\\s+\\|\\s+.+\\s+\\|\\s+.+\\s+\\|\\s+.+\\s*$"_s);
    static const QRegularExpression row(u"^\\s*(\\d+)\\s*\\|\\s*([0-9:.]+)\\s*\\|\\s*([0-9:.]+)\\s*\\|\\s*(\\d+)\\s*\\|\\s*(\\d+)\\s*$"_s);
    int i = 0;
    while (i < lines.size() && !header.match(lines.at(i)).hasMatch())
        ++i;
    AUDEX_CHECK(t, i < lines.size());
    i += 2; // the header and the separator line
    QList<int> starts;
    int lastEnd = -1;
    for (; i < lines.size(); ++i) {
        const QRegularExpressionMatch m = row.match(lines.at(i));
        if (!m.hasMatch())
            break;
        AUDEX_EQUAL(t, qint64(m.captured(1).toInt()), qint64(starts.size() + 1));
        starts << m.captured(4).toInt();
        lastEnd = m.captured(5).toInt();
    }
    AUDEX_EQUAL(t, qint64(starts.size()), qint64(3));

    QStringList picard = {u"1"_s, u"3"_s, QString::number(lastEnd + 150 + 1)};
    for (const int start : std::as_const(starts))
        picard << QString::number(start + 150);
    AUDEX_EQUAL(t, picard.join(u' '), Cdda::musicBrainzTocString(toc));
}
