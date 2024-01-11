/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CRC32HASH_HEADER
#define CRC32HASH_HEADER

#include <QByteArray>
#include <QString>

#include <KLocalizedString>

class CRC32Hash
{
public:
    CRC32Hash();
    CRC32Hash(const CRC32Hash &other);
    CRC32Hash &operator=(const CRC32Hash &other);

    bool operator==(const CRC32Hash &other) const;
    bool operator!=(const CRC32Hash &other) const;

    ~CRC32Hash();

    void addData(const QByteArray &ba);
    quint32 result() const;
    void clear();

    static quint32 crc32(const QByteArray &data)
    {
        quint32 crc = 0xFFFFFFFF;
        for (quint8 byte : data) {
            crc = (crc ^ byte) & 0xFFFFFFFF;
            for (int i = 0; i < 8; ++i)
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xEDB88320;
                else
                    crc >>= 1;
        }
        return crc ^ 0xFFFFFFFF;
    }

protected:
    quint32 p_crc;
};

#endif
