/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// CTDB recovery data against reference values of the original CUETools code
// (CUETools.AccurateRip / CUETools.Parity, built with .NET and fed with the
// same pseudo-random disc).

#include "test_framework.h"

#include "core/ctdbparity.h"

#include <QtEndian>

using namespace Audex;
using namespace Audex::Ctdb;

namespace
{

// TOC "32 700 1500 2345" in CUETools terms: 32 sectors before index 01 of
// track 1, three tracks, the audio ends at sector 2345
constexpr int Pregap = 32;
constexpr int Sectors = 2345;
constexpr int WordsPerSector = Cdda::SectorBytes / 2;

// the generator of the reference program (xorshift32)
QByteArray pcm(int sectors, quint32 seed)
{
    QByteArray data(qsizetype(sectors) * Cdda::SectorBytes, '\0');
    quint32 s = seed;
    for (qsizetype i = 0; i < data.size() / 2; ++i) {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        qToLittleEndian<quint16>(quint16(s >> 8), data.data() + 2 * i);
    }
    return data;
}

quint16 word(const QByteArray &data, qint64 index)
{
    return qFromLittleEndian<quint16>(data.constData() + 2 * index);
}

void xorWord(QByteArray &data, qint64 index, quint16 mask)
{
    qToLittleEndian<quint16>(word(data, index) ^ mask, data.data() + 2 * index);
}

// CUETools' Crc32.ComputeChecksum(0, ...): the plain CRC-32 register
quint32 registerCrc(QByteArrayView data)
{
    quint32 c = 0;
    for (const char byte : data) {
        c ^= quint8(byte);
        for (int k = 0; k < 8; ++k)
            c = (c & 1) ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    }
    return c;
}

Parity parityOf(const QByteArray &image)
{
    Parity parity(qint64(Pregap) * WordsPerSector, qint64(Sectors - Pregap) * WordsPerSector);
    for (qsizetype pos = 0; pos < image.size(); pos += 4099) // odd chunks on purpose
        parity.update(QByteArrayView(image).sliced(pos, std::min<qsizetype>(4099, image.size() - pos)));
    return parity;
}

QByteArray movedSyndromeData(const Parity &parity, int shift)
{
    QByteArray data(qsizetype(MaxParity) * Stride * 2, '\0');
    for (int column = 0; column < Stride; ++column) {
        const ColumnSyndromes s = parity.syndromes(column, shift);
        for (int i = 0; i < MaxParity; ++i)
            qToLittleEndian<quint16>(s[i], data.data() + (qsizetype(i) * Stride + column) * 2);
    }
    return data;
}

// layout of the reference: per correction the word (int32) and the mask (uint16)
quint32 correctionsCrc(const QList<Correction> &corrections)
{
    QByteArray data;
    for (const Correction &c : corrections) {
        char b[6];
        qToLittleEndian<qint32>(qint32(c.word), b);
        qToLittleEndian<quint16>(c.mask, b + 4);
        data.append(b, 6);
    }
    return registerCrc(data);
}

// the database pressing: its sample i is our sample i + pressing
QByteArray pressingOf(const QByteArray &clean, int pressing)
{
    QByteArray db(clean.size(), '\0');
    const qint64 words = clean.size() / 2;
    for (qint64 i = 0; i < words; ++i) {
        const qint64 j = i + 2 * qint64(pressing);
        if (j >= 0 && j < words)
            qToLittleEndian<quint16>(word(clean, j), db.data() + 2 * i);
    }
    return db;
}

QList<quint16> column0(const Parity &parity)
{
    const ColumnSyndromes s = parity.syndromes(0, 0);
    return QList<quint16>(s.cbegin(), s.cend());
}

}

AUDEX_TEST("CTDB syndromes and CRCs match CUETools")
{
    AUDEX_EQUAL(t, qint64(registerCrc("123456789")), qint64(0x2dfd2d88u));

    const Parity parity = parityOf(pcm(Sectors, 12345));
    AUDEX_CHECK(t, parity.isValid() && parity.isComplete());

    struct Expected {
        int shift;
        quint32 syndromes;
        quint32 crc;
        quint16 s00, s0_15, s11759_3;
    };
    const Expected expected[] = {
        {0, 0x8cbb2b03u, 0xe21c5a3fu, 27487, 56360, 24989},
        {1234, 0xe43c872bu, 0x6a43abe9u, 64765, 1306, 19028},
        {-777, 0x5cbea4dbu, 0xaf18d63au, 47574, 45515, 46532},
        {5879, 0x31ae74f5u, 0xcd6ff110u, 35455, 8927, 13038},
        {-5879, 0x9ced3e66u, 0x52fcc565u, 19503, 23041, 12226},
    };
    for (const Expected &e : expected) {
        AUDEX_EQUAL(t, qint64(registerCrc(movedSyndromeData(parity, e.shift))), qint64(e.syndromes));
        AUDEX_EQUAL(t, qint64(parity.crc(e.shift)), qint64(e.crc));
        AUDEX_EQUAL(t, qint64(parity.syndromes(0, e.shift)[0]), qint64(e.s00));
        AUDEX_EQUAL(t, qint64(parity.syndromes(0, e.shift)[15]), qint64(e.s0_15));
        AUDEX_EQUAL(t, qint64(parity.syndromes(11759, e.shift)[3]), qint64(e.s11759_3));
    }
    AUDEX_CHECK(t, parity.syndromeData(16) == movedSyndromeData(parity, 0));

    const QList<quint16> fromParity = Parity::syndromesFromParity({61560, 14609, 26422, 65135, 49172, 44678, 63231, 41730});
    AUDEX_CHECK(t, fromParity == QList<quint16>({27487, 45131, 29567, 6185, 48913, 31994, 10073, 38265}));
}

AUDEX_TEST("CTDB repair of scattered errors matches CUETools, also for other pressings")
{
    const QByteArray clean = pcm(Sectors, 12345);
    QByteArray rip = clean;
    for (int i = 0; i < 3 * WordsPerSector; ++i)
        qToLittleEndian<quint16>(0, rip.data() + 2 * (400 * WordsPerSector + 17 + i)); // 3 sectors of silence
    xorWord(rip, 1000 * WordsPerSector + 5, 0x1234);
    xorWord(rip, 1800 * WordsPerSector + 777, 0x8001);
    xorWord(rip, Pregap * WordsPerSector + 6 * Stride, 0x4242); // column 0 at shift 0
    const Parity ours = parityOf(rip);

    struct Expected {
        int pressing;
        quint32 crc;
    };
    for (const Expected &e : {Expected{0, 0xe21c5a3fu}, Expected{321, 0x64de87dau}, Expected{-4000, 0xd058df35u}}) {
        const Parity db = parityOf(pressingOf(clean, e.pressing));
        AUDEX_EQUAL(t, qint64(db.crc(0)), qint64(e.crc));

        const std::optional<ShiftMatch> match = ours.findShift(column0(db), db.crc(0));
        AUDEX_CHECK(t, match.has_value());
        if (!match)
            continue;
        AUDEX_EQUAL(t, match->shift, e.pressing);
        AUDEX_CHECK(t, match->errors);
        for (int npar : {16, 8}) {
            const auto fix = ours.corrections(db.syndromeData(16), npar, match->shift, db.crc(0));
            AUDEX_CHECK(t, fix.has_value());
            if (!fix)
                continue;
            AUDEX_EQUAL(t, fix->size(), 3531);
            AUDEX_EQUAL(t, qint64(correctionsCrc(*fix)), qint64(0x13e40c0fu));
            AUDEX_EQUAL(t, fix->first().word, qint64(108192));
            AUDEX_EQUAL(t, qint64(fix->first().mask), qint64(16962));
            AUDEX_EQUAL(t, fix->last().word, qint64(2117577));
            AUDEX_EQUAL(t, qint64(fix->last().mask), qint64(32769));

            QByteArray repaired = rip;
            for (const Correction &c : *fix)
                xorWord(repaired, c.word, c.mask);
            AUDEX_CHECK(t, repaired == clean);
        }
    }
}

AUDEX_TEST("CTDB repair capacity matches CUETools")
{
    const QByteArray clean = pcm(Sectors, 12345);
    struct Expected {
        const char *name;
        int burstSectors;
        int pressing;
        bool found;
        int shift;
        bool errors;
        int corrections16; // -1: not repairable
        quint32 crc16;
        int corrections8;
        int corrections4;
    };
    const Expected expected[] = {
        {"burst 35", 35, 0, true, 0, true, 41160, 0xf641f42au, 41160, -1},
        {"burst 60", 60, 0, true, 0, true, 70560, 0xca4aa02fu, -1, -1},
        {"burst 100", 100, 0, false, 0, true, -1, 0, -1, -1},
        {"clean, pressing 321", 0, 321, true, 321, false, -1, 0, -1, -1},
        {"burst 60, pressing -1000", 60, -1000, true, -1000, true, 70560, 0xca4aa02fu, -1, -1},
    };
    for (const Expected &e : expected) {
        const Parity db = parityOf(pressingOf(clean, e.pressing));
        QByteArray rip = clean;
        for (int i = 0; i < e.burstSectors * WordsPerSector; ++i)
            xorWord(rip, 900 * WordsPerSector + 3 + i, 0x5a5a);
        const Parity ours = parityOf(rip);

        const std::optional<ShiftMatch> match = ours.findShift(column0(db), db.crc(0));
        AUDEX_CHECK_MSG(t, match.has_value() == e.found, QString::fromLatin1(e.name));
        if (!match)
            continue;
        AUDEX_EQUAL(t, match->shift, e.shift);
        AUDEX_EQUAL(t, match->errors, e.errors);
        if (!match->errors)
            continue;
        const auto fix16 = ours.corrections(db.syndromeData(16), 16, match->shift, db.crc(0));
        const auto fix8 = ours.corrections(db.syndromeData(16), 8, match->shift, db.crc(0));
        const auto fix4 = ours.corrections(db.syndromeData(16), 4, match->shift, db.crc(0));
        AUDEX_EQUAL(t, fix16 ? int(fix16->size()) : -1, e.corrections16);
        AUDEX_EQUAL(t, fix8 ? int(fix8->size()) : -1, e.corrections8);
        AUDEX_EQUAL(t, fix4 ? int(fix4->size()) : -1, e.corrections4);
        if (fix16)
            AUDEX_EQUAL(t, qint64(correctionsCrc(*fix16)), qint64(e.crc16));
    }
}
