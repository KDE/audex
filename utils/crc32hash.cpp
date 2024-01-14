/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "crc32hash.h"

#include <QDebug>

#define MASK1 0x00FFFFFF
#define MASK2 0xFFFFFFFF
#define POLYNOMIAL 0xEDB88320

CRC32Hash::CRC32Hash(const quint32 initial_value)
{
    p_initial_value = initial_value;
    init(initial_value);
}

CRC32Hash::CRC32Hash(const CRC32Hash &other)
{
    crc32_accum = other.crc32_accum;
    memcpy(crc32_table, other.crc32_table, 256);
}

CRC32Hash &CRC32Hash::operator=(const CRC32Hash &other)
{
    crc32_accum = other.crc32_accum;
    memcpy(crc32_table, other.crc32_table, 256);
    return *this;
}

bool CRC32Hash::operator==(const CRC32Hash &other) const
{
    return result() == other.result();
}

bool CRC32Hash::operator!=(const CRC32Hash &other) const
{
    return result() != other.result();
}

CRC32Hash::~CRC32Hash()
{
}

void CRC32Hash::addData(const QByteArray &ba)
{
    int length = ba.length();
    const unsigned char *buffer = reinterpret_cast<const unsigned char *>(ba.constData());
    while (length-- > 0)
        crc32_accum = ((crc32_accum >> 8) & MASK1) ^ crc32_table[(crc32_accum & 0xff) ^ *buffer++];
}

quint32 CRC32Hash::result() const
{
    return (~crc32_accum) & MASK2;
}

void CRC32Hash::clear()
{
    crc32_accum = p_initial_value;
}

void CRC32Hash::init(const quint32 initial_value)
{
    crc32_accum = initial_value;

    for (int byte = 0; byte != 256; byte++) {
        quint32 data = byte;

        for (int i = 8; i > 0; --i)
            data = data & 1 ? (data >> 1) ^ POLYNOMIAL : data >> 1;

        crc32_table[byte] = data;
    }
}
