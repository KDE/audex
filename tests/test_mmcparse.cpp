/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "core/cdrmanufacturer.h"
#include "core/mmcparse.h"

using namespace Audex;

AUDEX_TEST("sense data carries the information field")
{
    QByteArray fixed(18, '\0');
    fixed[0] = char(0xF0); // valid, current error, fixed format
    fixed[2] = 0x01; // RECOVERED ERROR
    fixed[5] = 0x12;
    fixed[6] = 0x34;
    const Mmc::Sense a = Mmc::parseSense(fixed);
    AUDEX_CHECK(t, a.valid && a.informationValid);
    AUDEX_EQUAL(t, a.information, qint64(0x1234));

    QByteArray descriptor(20, '\0');
    descriptor[0] = 0x72; // descriptor format
    descriptor[1] = 0x01;
    descriptor[7] = 12; // additional length
    descriptor[8] = 0x00; // information descriptor
    descriptor[9] = 0x0A;
    descriptor[10] = char(0x80); // valid
    descriptor[18] = 0x12;
    descriptor[19] = 0x34;
    const Mmc::Sense b = Mmc::parseSense(descriptor);
    AUDEX_CHECK(t, b.valid && b.informationValid);
    AUDEX_EQUAL(t, b.information, qint64(0x1234));

    fixed[0] = 0x70; // information not valid
    AUDEX_CHECK(t, !Mmc::parseSense(fixed).informationValid);
}

// ATIP response (READ TOC/PMA/ATIP format 4): 4 byte header, 24 byte descriptor
static QByteArray atipResponse(bool rewritable, int m, int s, int f)
{
    QByteArray d(4 + 24, '\0');
    d[1] = char(2 + 24); // data length
    d[4 + 2] = char(rewritable ? 0x40 : 0x00);
    d[4 + 4] = char(m);
    d[4 + 5] = char(s);
    d[4 + 6] = char(f);
    d[4 + 8] = 79; // last possible lead-out 79:59:74
    d[4 + 9] = 59;
    d[4 + 10] = 74;
    return d;
}

AUDEX_TEST("ATIP: CD-R, CD-RW and the manufacturer")
{
    const auto cdr = Mmc::parseAtip(atipResponse(false, 97, 24, 1));
    AUDEX_CHECK(t, cdr.has_value());
    AUDEX_CHECK(t, !cdr->rewritable);
    AUDEX_EQUAL(t, cdr->leadInSecond, 24);
    AUDEX_EQUAL(t, cdr->leadOutMinute, 79);
    AUDEX_CHECK_MSG(t,
                    Mmc::cdrManufacturer(cdr->leadInMinute, cdr->leadInSecond, cdr->leadInFrame) == QStringLiteral("Taiyo Yuden Company Limited"),
                    "Taiyo Yuden at 97:24:01");

    const auto cdrw = Mmc::parseAtip(atipResponse(true, 97, 26, 66));
    AUDEX_CHECK(t, cdrw.has_value() && cdrw->rewritable);
    AUDEX_CHECK_MSG(t, Mmc::cdrManufacturer(97, 26, 66) == QStringLiteral("CMC Magnetics Corporation"), "CMC at 97:26:6x");

    AUDEX_CHECK(t, Mmc::cdrManufacturer(97, 99, 0).isEmpty());
    AUDEX_CHECK(t, Mmc::cdrManufacturer(97, 24, 11) == QStringLiteral("Sony Corporation"));
}

AUDEX_TEST("ATIP: nothing for a pressed CD")
{
    // header only, as some drives answer for a pressed CD
    QByteArray empty(4, '\0');
    empty[1] = 2;
    AUDEX_CHECK(t, !Mmc::parseAtip(empty).has_value());
    // a descriptor of zeros
    QByteArray zeros(4 + 24, '\0');
    zeros[1] = char(2 + 24);
    AUDEX_CHECK(t, !Mmc::parseAtip(zeros).has_value());
    // implausible times
    AUDEX_CHECK(t, !Mmc::parseAtip(atipResponse(false, 97, 61, 0)).has_value());
    AUDEX_CHECK(t, !Mmc::parseAtip(atipResponse(false, 97, 24, 75)).has_value());
    AUDEX_CHECK(t, !Mmc::parseAtip(QByteArray()).has_value());
}

AUDEX_TEST("GET CONFIGURATION: the current profile")
{
    QByteArray header(8, '\0');
    header[3] = 4;
    header[7] = 0x09;
    AUDEX_EQUAL(t, Mmc::parseCurrentProfile(header).value_or(0), quint16(0x0009));
    header[7] = 0x0A;
    AUDEX_EQUAL(t, Mmc::parseCurrentProfile(header).value_or(0), quint16(0x000A));
    AUDEX_CHECK(t, !Mmc::parseCurrentProfile(header.first(6)).has_value());
    header[3] = 0; // data length too short for the profile
    AUDEX_CHECK(t, !Mmc::parseCurrentProfile(header).has_value());
}

AUDEX_TEST("capabilities page: writer or reader")
{
    QByteArray d(8 + 20, '\0');
    d[1] = char(6 + 20); // mode data length
    d[8] = 0x2A;
    d[9] = 18;
    d[8 + 3] = 0x01; // CD-R write
    d[8 + 5] = 0x11; // CD-DA, C2 pointers
    auto c = Mmc::parseCapabilitiesPage(d);
    AUDEX_CHECK(t, c.has_value() && c->cdrWrite && c->c2Pointers);
    d[8 + 3] = 0x00;
    c = Mmc::parseCapabilitiesPage(d);
    AUDEX_CHECK(t, c.has_value() && !c->cdrWrite);
}
