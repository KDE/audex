/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "checksums.h"

#include "cdda.h"

#include <QtEndian>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>

namespace Audex::Rip
{

namespace
{

constexpr std::array<quint32, 256> makeCrc32Table()
{
    std::array<quint32, 256> table{};
    for (quint32 i = 0; i < 256; ++i) {
        quint32 c = i;
        for (int k = 0; k < 8; ++k)
            c = (c & 1) ? (c >> 1) ^ 0xEDB88320u : (c >> 1);
        table[i] = c;
    }
    return table;
}

constexpr std::array<quint32, 256> crc32Table = makeCrc32Table();

inline quint32 crc32Update(quint32 crc, const uchar *p, qsizetype n)
{
    while (n-- > 0)
        crc = crc32Table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return crc;
}

}

// ---- CRC-32 -----------------------------------------------------------------

void Crc32::update(QByteArrayView data)
{
    m_crc = crc32Update(m_crc, reinterpret_cast<const uchar *>(data.constData()), data.size());
}

quint32 Crc32::compute(QByteArrayView data)
{
    Crc32 c;
    c.update(data);
    return c.value();
}

ClippedCrc32::ClippedCrc32(qint64 samples, qint64 skipFirst, qint64 skipLast)
    : m_from(skipFirst * Cdda::BytesPerSample)
    , m_to(std::max(m_from, (samples - skipLast) * Cdda::BytesPerSample))
{
}

void ClippedCrc32::update(QByteArrayView data)
{
    const qint64 begin = m_position;
    m_position += data.size();
    const qint64 from = std::max(begin, m_from);
    const qint64 to = std::min(m_position, m_to);
    if (from < to)
        m_crc.update(data.sliced(from - begin, to - from));
}

namespace
{

// polynomial arithmetic modulo the CRC-32 polynomial, bit-reflected (zlib)
quint32 multModP(quint32 a, quint32 b)
{
    quint32 product = 0;
    for (quint32 m = 1u << 31; m; m >>= 1) {
        if (a & m)
            product ^= b;
        b = (b & 1) ? (b >> 1) ^ 0xEDB88320u : b >> 1;
    }
    return product;
}

// x^(8 * bytes) modulo the polynomial
quint32 shiftBytes(qint64 bytes)
{
    quint32 power = 1u << 31; // x^0
    quint32 square = 1u << 23; // x^8
    for (; bytes > 0; bytes >>= 1) {
        if (bytes & 1)
            power = multModP(square, power);
        square = multModP(square, square);
    }
    return power;
}

}

quint32 crc32Combine(quint32 crcA, quint32 crcB, qint64 bytesB)
{
    return multModP(shiftBytes(bytesB), crcA) ^ crcB;
}

quint32 crc32Xor(quint32 crc, qint64 length, qint64 position, QByteArrayView delta)
{
    // the CRC register is affine in the message: the change caused by delta
    // is its register value from zero, moved over the bytes that follow
    const quint32 change = crc32Update(0, reinterpret_cast<const uchar *>(delta.constData()), delta.size());
    return crc ^ multModP(shiftBytes(length - position - delta.size()), change);
}

QList<quint32> slidingCrc32(quint32 crc, qint64 length, QByteArrayView head, QByteArrayView tail, int range)
{
    constexpr int S = Cdda::BytesPerSample;
    const qsizetype edge = qsizetype(2) * range * S;
    if (range < 0 || length < S || head.size() < edge || tail.size() < edge)
        return {};
    const auto sample = [](QByteArrayView pcm, int i) {
        return Crc32::compute(pcm.sliced(qsizetype(i) * S, S));
    };
    const quint32 rest = shiftBytes(length - S); // crc(A + B) = rest * crc(A) ^ crc(B), |B| = length - S
    const quint32 step = shiftBytes(S); // crc(B + C) = step * crc(B) ^ crc(C), |C| = S
    quint32 back = 1u << 31; // step^-1 = x^-32; x^-1 = 0xDB710641
    for (int i = 0; i < 8 * S; ++i)
        back = multModP(back, 0xDB710641u);

    QList<quint32> result(2 * range + 1);
    result[range] = crc;
    quint32 c = crc;
    for (int s = 0; s < range; ++s) { // drop the first sample, append one
        const quint32 middle = c ^ multModP(rest, sample(head, range + s));
        c = multModP(step, middle) ^ sample(tail, range + s);
        result[range + s + 1] = c;
    }
    c = crc;
    for (int s = 0; s > -range; --s) { // drop the last sample, prepend one
        const quint32 middle = multModP(back, c ^ sample(tail, range + s - 1));
        c = multModP(rest, sample(head, range + s - 1)) ^ middle;
        result[range + s - 1] = c;
    }
    return result;
}

// ---- CRC-32 without null samples ---------------------------------------------

void Crc32NoNull::reset()
{
    m_crc = 0xFFFFFFFFu;
    m_hasPending = false;
}

void Crc32NoNull::update(QByteArrayView data)
{
    qsizetype i = 0;
    if (m_hasPending && !data.isEmpty()) {
        const uchar s[2] = {uchar(m_pending), uchar(data.at(0))};
        if (s[0] | s[1])
            m_crc = crc32Update(m_crc, s, 2);
        m_hasPending = false;
        i = 1;
    }
    const auto *p = reinterpret_cast<const uchar *>(data.constData());
    for (; i + 1 < data.size(); i += 2) {
        if (p[i] | p[i + 1])
            m_crc = crc32Update(m_crc, p + i, 2);
    }
    if (i < data.size()) {
        m_pending = data.at(i);
        m_hasPending = true;
    }
}

// ---- AccurateRip ------------------------------------------------------------

AccurateRipChecksum::AccurateRipChecksum(qint64 trackSamples, bool firstTrack, bool lastTrack)
{
    const qint64 skip = 5 * Cdda::SamplesPerSector;
    m_checkFrom = firstTrack ? skip : 1;
    m_checkTo = lastTrack ? trackSamples - skip : trackSamples;
}

inline void AccurateRipChecksum::processSample(quint32 sample)
{
    const quint32 m = quint32(m_multiplier);
    if (m_multiplier >= m_checkFrom && m_multiplier <= m_checkTo) {
        const quint64 product = quint64(sample) * quint64(m);
        const quint32 lo = quint32(product);
        const quint32 hi = quint32(product >> 32);
        m_v1 += lo;
        m_v2 += lo + hi;
    }
    // frame 450 CRC: sector 450 of the track, multiplier restarts at 1
    const qint64 f450First = 450 * Cdda::SamplesPerSector + 1;
    if (m_multiplier >= f450First && m_multiplier < f450First + Cdda::SamplesPerSector)
        m_frame450 += sample * quint32(m_multiplier - f450First + 1);
    ++m_multiplier;
}

void AccurateRipChecksum::update(QByteArrayView data)
{
    const char *p = data.constData();
    qsizetype n = data.size();

    if (m_pendingLength > 0) {
        const int needed = 4 - m_pendingLength;
        const int toCopy = std::min<qsizetype>(n, needed);
        std::memcpy(m_pending + m_pendingLength, p, toCopy);
        m_pendingLength += toCopy;
        p += toCopy;
        n -= toCopy;

        if (m_pendingLength < 4) {
            return;
        }

        processSample(qFromLittleEndian<quint32>(m_pending));
        m_pendingLength = 0;
    }

    while (n >= 4) {
        processSample(qFromLittleEndian<quint32>(p));
        p += 4;
        n -= 4;
    }

    if (n > 0) {
        std::memcpy(m_pending, p, n);
        m_pendingLength = static_cast<int>(n);
    }
}

QList<quint32> frame450Checksums(QByteArrayView pcm, int range)
{
    QList<quint32> result;
    const qsizetype samples = pcm.size() / Cdda::BytesPerSample;
    if (range < 0 || samples < Cdda::SamplesPerSector + 2 * qsizetype(range))
        return result;
    result.reserve(2 * range + 1);
    for (int shift = -range; shift <= range; ++shift) {
        const char *p = pcm.constData() + qsizetype(range + shift) * Cdda::BytesPerSample;
        quint32 crc = 0;
        for (quint32 k = 1; k <= quint32(Cdda::SamplesPerSector); ++k, p += Cdda::BytesPerSample)
            crc += k * qFromLittleEndian<quint32>(p);
        result.append(crc);
    }
    return result;
}

// ---- Peak -----------------------------------------------------------------

void PeakMeter::update(QByteArrayView data)
{
    qsizetype i = 0;
    auto take = [this](qint16 v) {
        const int a = std::abs(int(v));
        if (a > m_peak)
            m_peak = a;
    };
    if (m_hasPending && !data.isEmpty()) {
        const char s[2] = {m_pending, data.at(0)};
        take(qFromLittleEndian<qint16>(s));
        m_hasPending = false;
        i = 1;
    }
    for (; i + 1 < data.size(); i += 2)
        take(qFromLittleEndian<qint16>(data.constData() + i));
    if (i < data.size()) {
        m_pending = data.at(i);
        m_hasPending = true;
    }
}

}
