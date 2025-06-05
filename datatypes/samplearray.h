/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>

namespace Audex
{

// Signed Little-Endian 16bit-Integer, stereo interleaved, first left channel, second right channel
class SampleArray
{
public:
    explicit SampleArray(const qsizetype size = 0); // in samples
    explicit SampleArray(const QByteArray &data);
    explicit SampleArray(int16_t *samples, const qsizetype size);

    const qint16 &at(qsizetype i) const;
    qint16 operator[](const qsizetype i) const;
    qint16 operator[](const qsizetype i);

    void append(const SampleArray &sampleArray);
    void appendZeroSamples(qsizetype n);

    qsizetype size() const; // in samples
    qsizetype sizeBytes() const;
    bool isEmpty() const;
    void clear();

    const QByteArray &data() const;

    // prepend headArray and cut the same amount of samples at the end
    SampleArray shiftLeft(const SampleArray &headArray) const;

    // append tailArray and cut the same amount of samples at the beginning
    SampleArray shiftRight(const SampleArray &tailArray) const;

    SampleArray right(const qsizetype len) const;
    SampleArray left(const qsizetype len) const;

    quint32 crc32() const;

private:
    QByteArray p_data;
};

}
