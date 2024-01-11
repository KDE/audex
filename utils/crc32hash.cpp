/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "crc32hash.h"

#include <QDebug>

CRC32Hash::CRC32Hash()
{
    clear();
}

CRC32Hash::CRC32Hash(const CRC32Hash &other)
{
    p_crc = other.p_crc;
}

CRC32Hash &CRC32Hash::operator=(const CRC32Hash &other)
{
    p_crc = other.p_crc;
    return *this;
}

bool CRC32Hash::operator==(const CRC32Hash &other) const
{
    return p_crc == other.p_crc;
}

bool CRC32Hash::operator!=(const CRC32Hash &other) const
{
    return p_crc != other.p_crc;
}

CRC32Hash::~CRC32Hash()
{
}

void CRC32Hash::addData(const QByteArray &ba)
{
    for (quint8 byte : ba) {
        p_crc = (p_crc ^ byte) & 0xFFFFFFFF;
        for (int i = 0; i < 8; ++i)
            if (p_crc & 1)
                p_crc = (p_crc >> 1) ^ 0xEDB88320;
            else
                p_crc >>= 1;
    }
}

quint32 CRC32Hash::result() const
{
    return p_crc ^ 0xFFFFFFFF;
}

void CRC32Hash::clear()
{
    p_crc = 0xFFFFFFFF;
}
