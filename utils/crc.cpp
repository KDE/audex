/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "crc.h"

namespace CRC
{

CRC16_X25_Calculator::CRC16_X25_Calculator(const quint16 initialValue)
{
    reset(initialValue);
}

quint16 CRC16_X25_Calculator::result() const
{
    return crc;
}

QString CRC16_X25_Calculator::resultHexString() const
{
    return QStringLiteral("%1").arg(crc, 4, 16, QChar(u'0'));
}

void CRC16_X25_Calculator::reset(const quint16 initialValue)
{
    crc = initialValue;
}

void CRC16_X25_Calculator::process(const QByteArray &data)
{
    crc = calc(data, crc);
}

bool CRC16_X25_Calculator::processFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QDataStream in(&file);
    while (!file.atEnd())
        process(file.read(CRC_FILE_BUFSIZE));
    file.close();
    return true;
}

quint16 CRC16_X25_Calculator::calc(const QByteArray &data, const quint16 initialValue)
{
    quint16 crc = initialValue;
    int length = data.length();
    const unsigned char *buffer = reinterpret_cast<const unsigned char *>(data.constData());
    while (length-- > 0)
        crc = (crc << 8) ^ x25_table[(crc >> 8) ^ *buffer++];
    return crc;
}

CRC32_Calculator::CRC32_Calculator(const quint32 initialValue)
{
    reset(initialValue);
}

quint32 CRC32_Calculator::result() const
{
    return ~crc & CRC32_MASK2;
}

QString CRC32_Calculator::resultHexString() const
{
    return QStringLiteral(u"%1").arg(result(), 8, 16, QChar(u'0'));
}

void CRC32_Calculator::reset(const quint32 initialValue)
{
    crc = initialValue;
}

void CRC32_Calculator::process(const QByteArray &data)
{
    crc = calc(data, crc);
}

bool CRC32_Calculator::processFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    while (!file.atEnd())
        process(file.read(CRC_FILE_BUFSIZE));
    file.close();
    return true;
}

quint32 CRC32_Calculator::calc(const QByteArray &data, const quint32 initialValue)
{
    quint32 crc = initialValue;
    int length = data.length();
    const unsigned char *buffer = reinterpret_cast<const unsigned char *>(data.constData());
    while (length-- > 0)
        crc = ((crc >> 8) & CRC32_MASK1) ^ crc32_table[(crc & 0xFF) ^ *buffer++];
    return crc;
}

CRC32_PCM_Calculator::CRC32_PCM_Calculator(const quint32 initialValue)
{
    reset(initialValue);
}

quint32 CRC32_PCM_Calculator::result() const
{
    return ~crc & CRC32_MASK2;
}

QString CRC32_PCM_Calculator::resultHexString() const
{
    return QStringLiteral("%1").arg(result(), 8, 16, QChar(u'0'));
}

void CRC32_PCM_Calculator::reset(const quint32 initialValue)
{
    crc = initialValue;
}

void CRC32_PCM_Calculator::process(const QByteArray &data)
{
    crc = calc(data, crc);
}

quint32 CRC32_PCM_Calculator::calc(const QByteArray &data, const quint32 initialValue)
{
    if (data.length() % 2)
        return 0;

    quint32 crc = initialValue;
    int length = data.length();
    const unsigned char *buffer = reinterpret_cast<const unsigned char *>(data.constData());
    while (length > 0) {
        crc = crc32_table[(crc ^ buffer[0]) & 0xff] ^ (crc >> 8);
        crc = crc32_table[(crc ^ buffer[1]) & 0xff] ^ (crc >> 8);
        buffer += 2;
        length -= 2;
    }
    return crc;
}

}
