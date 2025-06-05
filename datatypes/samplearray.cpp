/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "samplearray.h"

#include "utils/crc.h"

namespace Audex
{

SampleArray::SampleArray(qsizetype size)
    : p_data(size * sizeof(int16_t), 0) // initialize with zero bytes
{
}

SampleArray::SampleArray(const QByteArray &data)
{
    p_data = data;
}

SampleArray::SampleArray(int16_t *samples, const qsizetype size)
{
    if (samples)
        p_data = QByteArray(reinterpret_cast<const char *>(samples), size * sizeof(int16_t));
}

const int16_t &SampleArray::at(qsizetype i) const
{
    Q_ASSERT(i >= 0 && i < size());
    return *reinterpret_cast<const int16_t *>(p_data.constData() + i * sizeof(int16_t));
}

int16_t SampleArray::operator[](qsizetype i) const
{
    return at(i);
}

int16_t SampleArray::operator[](qsizetype i)
{
    Q_ASSERT(i >= 0 && i < size());
    return *reinterpret_cast<int16_t *>(p_data.data() + i * sizeof(int16_t));
}

void SampleArray::append(const SampleArray &sampleArray)
{
    p_data.append(sampleArray.data());
}

void SampleArray::appendZeroSamples(qsizetype n)
{
    if (n > 0)
        p_data.append(QByteArray(n * sizeof(int16_t), 0));
}

qsizetype SampleArray::size() const
{
    return p_data.size() / sizeof(int16_t);
}

qsizetype SampleArray::sizeBytes() const
{
    return p_data.size();
}

bool SampleArray::isEmpty() const
{
    return p_data.isEmpty();
}

void SampleArray::clear()
{
    p_data.clear();
}

const QByteArray &SampleArray::data() const
{
    return p_data;
}

SampleArray SampleArray::shiftLeft(const SampleArray &headArray) const
{
    SampleArray result;
    result = SampleArray(headArray);
    result.append(left(size() - headArray.size()));
    return result;
}

SampleArray SampleArray::shiftRight(const SampleArray &tailArray) const
{
    SampleArray result;
    result = SampleArray(right(size() - tailArray.size()));
    result.append(tailArray);
    return result;
}

SampleArray SampleArray::right(const qsizetype len) const
{
    return SampleArray(p_data.right(len * sizeof(int16_t)));
}

SampleArray SampleArray::left(const qsizetype len) const
{
    return SampleArray(p_data.left(len * sizeof(int16_t)));
}

quint32 SampleArray::crc32() const
{
    return CRC::CRC32_Calculator::calc(p_data);
}

}
