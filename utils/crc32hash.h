/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* This code is based on the CRC32 calculation class of the Krusader project
 * (https://invent.kde.org/utilities/krusader)
 */

#ifndef CRC32HASH_HEADER
#define CRC32HASH_HEADER

#include <QByteArray>
#include <QString>
#include <QVector>

#include <KLocalizedString>

class CRC32Hash
{
public:
    CRC32Hash(const quint32 initial_value = -1);
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
        CRC32Hash crc32;
        crc32.addData(data);
        return crc32.result();
    }

protected:
    quint32 p_initial_value;

    quint32 crc32_accum;
    quint32 crc32_table[256];

    void init(const quint32 initial_value);
};

#endif
