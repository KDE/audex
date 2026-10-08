/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * SPDX-FileCopyrightText: Copyright (C) 2008-2025 Gregory S. Chudov
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ctdbparity.h"

#include <QtEndian>

#include <algorithm>
#include <cstring>

namespace Audex::Ctdb
{

namespace
{

constexpr int Max = 0xFFFF; // multiplicative order of GF(2^16)

// GF(2^16) with the polynomial x^16 + x^12 + x^3 + x + 1, generator x
struct Field {
    std::vector<quint16> exp; // 2 * Max entries: exp[a + b] needs no reduction
    std::vector<int> log; // log[0] = -1

    Field()
        : exp(2 * Max)
        , log(Max + 1, -1)
    {
        int d = 1;
        for (int i = 0; i < Max; ++i) {
            exp[i] = exp[Max + i] = quint16(d);
            log[d] = i;
            d <<= 1;
            if (d & 0x10000)
                d = (d ^ 0x1100B) & Max;
        }
    }

    quint16 mul(quint16 a, quint16 b) const
    {
        return a && b ? exp[log[a] + log[b]] : 0;
    }
    quint16 mulExp(quint16 a, int e) const // a * x^e, 0 <= e < Max
    {
        return a ? exp[log[a] + e] : 0;
    }
    quint16 div(quint16 a, quint16 b) const
    {
        return a ? exp[log[a] - log[b] + Max] : 0;
    }
};

const Field &field()
{
    static const Field f;
    return f;
}

// Encoder tables: the feedback word times the generator polynomial, split into
// the low and the high byte of the feedback, 16 parity words packed in 4 lanes
struct EncodeTable {
    std::array<std::array<quint64, 4>, 256> low{};
    std::array<std::array<quint64, 4>, 256> high{};

    EncodeTable()
    {
        const Field &f = field();
        std::array<quint16, MaxParity> g{}; // generator with the roots x^0 .. x^15
        g[MaxParity - 1] = 1;
        for (int i = 0; i < MaxParity; ++i) {
            const quint16 root = f.exp[i];
            for (int j = 0; j < MaxParity - 1; ++j)
                g[j] = f.mul(g[j], root) ^ g[j + 1];
            g[MaxParity - 1] = f.mul(g[MaxParity - 1], root);
        }
        for (int b = 0; b < 256; ++b) {
            for (int i = 0; i < MaxParity; ++i) {
                low[b][i / 4] |= quint64(f.mul(quint16(b), g[i])) << (16 * (i % 4));
                high[b][i / 4] |= quint64(f.mul(quint16(b << 8), g[i])) << (16 * (i % 4));
            }
        }
    }
};

const EncodeTable &encodeTable()
{
    static const EncodeTable t;
    return t;
}

// Berlekamp-Massey: error locator sigma (sigma[0] = 1) of the syndromes,
// returns its degree or -1
int errorLocator(const quint16 *syn, int npar, std::array<quint16, MaxParity + 1> &sigma)
{
    const Field &f = field();
    std::array<quint16, MaxParity + 1> b{};
    sigma.fill(0);
    sigma[0] = b[0] = 1;
    int length = 0;
    int m = 1;
    quint16 lastD = 1;
    for (int n = 0; n < npar; ++n) {
        quint16 d = syn[n];
        for (int i = 1; i <= length; ++i)
            d ^= f.mul(sigma[i], syn[n - i]);
        if (d == 0) {
            ++m;
            continue;
        }
        const quint16 scale = f.div(d, lastD);
        const std::array<quint16, MaxParity + 1> previous = sigma;
        for (int i = 0; i + m <= MaxParity; ++i)
            sigma[i + m] ^= f.mul(scale, b[i]);
        if (2 * length <= n) {
            length = n + 1 - length;
            b = previous;
            lastD = d;
            m = 1;
        } else {
            ++m;
        }
    }
    return length > 0 && sigma[length] ? length : -1;
}

// Chien search: the positions (degrees 0 .. n - 1) of the errors; false
// unless all `count` roots of sigma are among them
bool errorPositions(const std::array<quint16, MaxParity + 1> &sigma, int count, int n, std::array<int, MaxParity> &positions)
{
    const Field &f = field();
    if (count == 1) { // sigma = 1 + s1 z: the error is at x^log(s1)
        positions[0] = f.log[sigma[1]];
        return positions[0] < n;
    }
    std::array<int, MaxParity + 1> term{}; // log of sigma_j * x^(-j * position)
    for (int j = 1; j <= count; ++j)
        term[j] = f.log[sigma[j]];
    int found = 0;
    for (int position = 0; position < n; ++position) {
        quint16 sum = 1;
        for (int j = 1; j <= count; ++j) {
            if (term[j] < 0)
                continue;
            sum ^= f.exp[term[j]];
            term[j] -= j;
            if (term[j] < 0)
                term[j] += Max;
        }
        if (sum == 0) {
            positions[found++] = position;
            if (found == count)
                return true;
        }
    }
    return false;
}

// Forney: the error value at x^position
quint16 errorValue(const quint16 *syn, const std::array<quint16, MaxParity + 1> &sigma, int count, int position)
{
    const Field &f = field();
    const int inverse = (Max - position) % Max; // log of z = x^-position
    quint16 omega = 0; // omega(z), omega = syn * sigma mod z^count
    for (int k = 0; k < count; ++k) {
        quint16 coefficient = 0;
        for (int j = 0; j <= k; ++j)
            coefficient ^= f.mul(sigma[j], syn[k - j]);
        omega ^= f.mulExp(coefficient, int(qint64(inverse) * k % Max));
    }
    quint16 derivative = 0; // sigma'(z)
    for (int j = 1; j <= count; j += 2)
        derivative ^= f.mulExp(sigma[j], int(qint64(inverse) * (j - 1) % Max));
    if (derivative == 0)
        return 0;
    return f.mulExp(f.div(omega, derivative), position);
}

}

Parity::Parity(qint64 offsetWords, qint64 words)
    : m_offset(offsetWords * 2)
    , m_words(words)
{
    if (words < 3 * Stride || words % 2 != 0)
        return;
    m_rows = int(words / Stride - 2);
    const qint64 lastStride = Stride + words % Stride;
    m_regionBegin = m_offset + qint64(Stride) * 2;
    m_regionEnd = m_regionBegin + qint64(m_rows) * Stride * 2;
    m_tailBegin = m_offset + (words - Stride - lastStride) * 2;
    m_lead = QByteArray(qsizetype(Stride) * 4, '\0');
    m_tail = QByteArray(qsizetype(Stride + lastStride) * 2, '\0');
    m_parity.assign(size_t(Stride) * 4, 0);
}

bool Parity::isValid() const
{
    // CUETools: the positions of a column must fit into the field
    return m_rows > 0 && m_rows + 2 + MaxParity <= Max;
}

bool Parity::isComplete() const
{
    return m_complete;
}

void Parity::update(QByteArrayView pcm)
{
    if (!isValid() || m_complete)
        return;
    const qint64 begin = m_position;
    const qint64 end = begin + pcm.size();
    m_position = end;

    const auto copy = [&](QByteArray &buffer, qint64 at) {
        const qint64 from = std::max(at, begin);
        const qint64 to = std::min(at + buffer.size(), end);
        if (from < to)
            std::memcpy(buffer.data() + (from - at), pcm.constData() + (from - begin), to - from);
    };
    copy(m_lead, m_offset);
    copy(m_tail, m_tailBegin);

    const qint64 from = std::max(m_regionBegin, begin);
    const qint64 to = std::min(m_regionEnd, end);
    if (from < to) {
        const QByteArrayView region = pcm.sliced(from - begin, to - from);
        m_crc.update(region);
        feed(region, from);
    }

    if (m_position >= m_offset + m_words * 2)
        finish();
}

void Parity::feed(QByteArrayView data, qint64 at)
{
    const EncodeTable &table = encodeTable();
    const char *p = data.constData();
    qsizetype n = data.size();
    qint64 word = (at - m_regionBegin) / 2;

    auto encode = [&](quint16 value) {
        quint64 *lanes = m_parity.data() + size_t(word % Stride) * 4;
        const quint16 feedback = quint16(lanes[0]) ^ value;
        const auto &lo = table.low[feedback & 0xFF];
        const auto &hi = table.high[feedback >> 8];
        lanes[0] = ((lanes[0] >> 16) | (lanes[1] << 48)) ^ lo[0] ^ hi[0];
        lanes[1] = ((lanes[1] >> 16) | (lanes[2] << 48)) ^ lo[1] ^ hi[1];
        lanes[2] = ((lanes[2] >> 16) | (lanes[3] << 48)) ^ lo[2] ^ hi[2];
        lanes[3] = (lanes[3] >> 16) ^ lo[3] ^ hi[3];
        ++word;
    };

    if (m_pending >= 0 && n > 0) {
        encode(quint16(m_pending | (quint8(*p) << 8)));
        m_pending = -1;
        ++p;
        --n;
    }
    for (; n >= 2; p += 2, n -= 2)
        encode(qFromLittleEndian<quint16>(p));
    if (n == 1)
        m_pending = quint8(*p);
}

void Parity::finish()
{
    m_complete = true;
    const Field &f = field();

    // syndrome x of a column from its parity words p: sum p[i] * x^(-(i + 1) * x)
    m_syndromes.assign(Stride, ColumnSyndromes{});
    for (int column = 0; column < Stride; ++column) {
        const quint64 *lanes = m_parity.data() + size_t(column) * 4;
        for (int i = 0; i < MaxParity; ++i) {
            const quint16 p = quint16(lanes[i / 4] >> (16 * (i % 4)));
            if (!p)
                continue;
            for (int x = 0; x < MaxParity; ++x)
                m_syndromes[column][x] ^= f.exp[(f.log[p] + Max - (i + 1) * x % Max) % Max];
        }
    }

    // CRCs of the moved windows: the PCM around the start of the window is in
    // the lead, around its end in the tail (both one sample in)
    const qint64 length = m_regionEnd - m_regionBegin;
    const qsizetype edge = qsizetype(2) * ShiftRange * Cdda::BytesPerSample;
    m_crcs = Rip::slidingCrc32(m_crc.value(),
                               length,
                               QByteArrayView(m_lead).sliced(Cdda::BytesPerSample, edge),
                               QByteArrayView(m_tail).sliced(Cdda::BytesPerSample, edge),
                               ShiftRange);
}

quint16 Parity::leadWord(qint64 index) const
{
    return qFromLittleEndian<quint16>(m_lead.constData() + index * 2);
}

quint16 Parity::tailWord(qint64 index) const
{
    return qFromLittleEndian<quint16>(m_tail.constData() + index * 2);
}

quint32 Parity::crc(int shift) const
{
    return m_crcs.value(shift + ShiftRange);
}

ColumnSyndromes Parity::syndromes(int column, int shift) const
{
    const Field &f = field();
    const int moved = column + 2 * shift;
    if (moved >= 0 && moved < Stride)
        return m_syndromes.at(moved);

    // The moved column is another column of ours, one row later or earlier:
    // take out the row that falls off and add the one that comes in
    ColumnSyndromes s{};
    if (moved >= Stride) {
        const int q = moved - Stride;
        const quint16 first = leadWord(Stride + q); // row 0
        const quint16 next = tailWord(Stride + q); // row rows()
        for (int i = 0; i < MaxParity; ++i)
            s[i] = f.mulExp(m_syndromes.at(q)[i], i) ^ next ^ f.mulExp(first, int(qint64(i) * m_rows % Max));
    } else {
        const int q = moved + Stride;
        const quint16 before = leadWord(q); // row -1
        const quint16 last = tailWord(q); // row rows() - 1
        for (int i = 0; i < MaxParity; ++i) {
            const quint16 v = m_syndromes.at(q)[i] ^ last ^ f.mulExp(before, int(qint64(i) * m_rows % Max));
            s[i] = f.mulExp(v, (Max - i) % Max);
        }
    }
    return s;
}

QByteArray Parity::syndromeData(int npar) const
{
    npar = std::clamp(npar, 0, MaxParity);
    QByteArray data(qsizetype(npar) * Stride * 2, '\0');
    for (int i = 0; i < npar; ++i)
        for (int column = 0; column < Stride; ++column)
            qToLittleEndian<quint16>(m_syndromes.at(column)[i], data.data() + (qsizetype(i) * Stride + column) * 2);
    return data;
}

std::optional<ShiftMatch> Parity::findShift(const QList<quint16> &column0, quint32 crc) const
{
    const int npar = int(std::min<qsizetype>(column0.size(), MaxParity));
    if (!m_complete || npar < 2)
        return std::nullopt;

    int bestErrors = npar / 2;
    int bestShift = 0;
    std::array<quint16, MaxParity> syn{};
    std::array<quint16, MaxParity + 1> sigma{};
    std::array<int, MaxParity> positions{};
    // in the order of CUETools, which decides between equally good shifts
    for (int shift = ShiftRange; shift >= -ShiftRange; --shift) {
        const ColumnSyndromes ours = syndromes(0, shift);
        bool differs = false;
        for (int i = 0; i < npar; ++i) {
            syn[i] = ours[i] ^ column0.at(i);
            differs = differs || syn[i];
        }
        if (!differs)
            return ShiftMatch{shift, this->crc(shift) != crc};
        const int errors = errorLocator(syn.data(), npar, sigma);
        if (errors > 0 && errors < bestErrors && errorPositions(sigma, errors, m_rows, positions)) {
            bestErrors = errors;
            bestShift = shift;
        }
    }
    if (bestErrors < npar / 2)
        return ShiftMatch{bestShift, true};
    return std::nullopt;
}

std::optional<QList<Correction>> Parity::corrections(QByteArrayView data, int npar, int shift, quint32 crc) const
{
    npar = std::min(npar, MaxParity);
    if (!m_complete || npar < 2 || data.size() < qsizetype(npar) * Stride * 2 || std::abs(shift) > ShiftRange)
        return std::nullopt;

    const qint64 first = m_offset / 2;
    const qint64 windowStart = first + Stride + 2 * qint64(shift);
    const qint64 windowBytes = m_regionEnd - m_regionBegin;
    quint32 corrected = this->crc(shift);

    QList<Correction> result;
    std::array<quint16, MaxParity> syn{};
    std::array<quint16, MaxParity + 1> sigma{};
    std::array<int, MaxParity> positions{};
    for (int column = 0; column < Stride; ++column) {
        const ColumnSyndromes ours = syndromes(column, shift);
        bool differs = false;
        for (int i = 0; i < npar; ++i) {
            syn[i] = ours[i] ^ qFromLittleEndian<quint16>(data.constData() + (qsizetype(i) * Stride + column) * 2);
            differs = differs || syn[i];
        }
        if (!differs)
            continue;
        const int errors = errorLocator(syn.data(), npar, sigma);
        if (errors <= 0 || errors > npar / 2 || !errorPositions(sigma, errors, m_rows, positions))
            return std::nullopt;
        for (int k = 0; k < errors; ++k) {
            const int row = m_rows - 1 - positions[k];
            const qint64 word = windowStart + qint64(row) * Stride + column;
            const quint16 mask = errorValue(syn.data(), sigma, errors, positions[k]);
            if (word < first || word >= first + m_words || mask == 0)
                return std::nullopt;
            char bytes[2];
            qToLittleEndian<quint16>(mask, bytes);
            corrected = Rip::crc32Xor(corrected, windowBytes, (word - windowStart) * 2, QByteArrayView(bytes, 2));
            result.append(Correction{word, mask});
        }
    }
    if (corrected != crc)
        return std::nullopt;
    std::sort(result.begin(), result.end(), [](const Correction &a, const Correction &b) {
        return a.word < b.word;
    });
    return result;
}

QList<quint16> Parity::syndromesFromParity(const QList<quint16> &parity)
{
    const Field &f = field();
    const int n = int(std::min<qsizetype>(parity.size(), MaxParity));
    QList<quint16> syn(n, 0);
    for (int i = 0; i < n; ++i) {
        const quint16 p = parity.at(i);
        if (!p)
            continue;
        for (int x = 0; x < n; ++x)
            syn[x] ^= f.exp[(f.log[p] + Max - (i + 1) * x % Max) % Max];
    }
    return syn;
}

}
