/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QDebug>
#include <QVector>

#include <QtEndian>

#include <KLocalizedString>

extern "C" {
#include <errno.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <scsi/sg.h>
#include <string.h>
#include <sys/ioctl.h>
}

#define MAX_SENSEBUFFER_LENGTH 32

namespace Audex
{

namespace Device
{

namespace SCSI
{

#define SECTOR_SIZE_BYTES 2352
#define SECTOR_SIZE_SAMPLES 588
#define C2_SIZE_BYTES 294
#define RAW_PW_SUBCHANNEL_SIZE_BYTES 96
#define CORRECTED_PW_SUBCHANNEL_SIZE_BYTES 96
#define FORMATED_Q_SUBCHANNEL_SIZE_BYTES 16

const quint8 MMC_INQUIRY = 0x12;
const quint8 MMC_READ_SUB_CHANNEL = 0x42;
const quint8 MMC_READ_TOC_PMA_ATIP = 0x43;
const quint8 MMC_MODE_SENSE = 0x5A;
const quint8 MMC_READ_CD = 0xBE;

class ErrorCode
{
public:
    explicit ErrorCode(const quint8 errorCode = 0, const quint8 senseKey = 0, const quint8 asc = 0, const quint8 ascq = 0)
    {
        p_error_code = errorCode;
        p_sense_key = senseKey;
        p_asc = asc;
        p_ascq = ascq;
    }
    ErrorCode(const ErrorCode &other)
    {
        p_error_code = other.p_error_code;
        p_sense_key = other.p_sense_key;
        p_asc = other.p_asc;
        p_ascq = other.p_ascq;
    }
    ErrorCode &operator=(const ErrorCode &other)
    {
        p_error_code = other.p_error_code;
        p_sense_key = other.p_sense_key;
        p_asc = other.p_asc;
        p_ascq = other.p_ascq;
        return *this;
    }

    bool operator==(const ErrorCode &other) const
    {
        return (p_error_code == other.p_error_code && p_sense_key == other.p_sense_key && p_asc == other.p_asc && p_ascq == other.p_ascq);
    }
    bool operator!=(const ErrorCode &other) const
    {
        return !(*this == other);
    }

    operator bool() const
    {
        return p_error_code != 0;
    }

    void clear()
    {
        p_error_code = 0;
        p_sense_key = 0;
        p_asc = 0;
        p_ascq = 0;
    }

    quint8 errorCode() const
    {
        return p_error_code;
    }

    quint8 senseKey() const
    {
        return p_sense_key;
    }

    quint8 asc() const
    {
        return p_asc;
    }

    quint8 ascq() const
    {
        return p_ascq;
    }

    QString senseKeyString() const;

private:
    quint8 p_error_code;
    quint8 p_sense_key;
    quint8 p_asc;
    quint8 p_ascq;
};

QDebug operator<<(QDebug debug, const ErrorCode &errcode);

class ScsiCommand
{
public:
    ScsiCommand(const int deviceHandle, const int cmdLength);

    quint8 &operator[](const int i);

    int send(QByteArray *buffer, const int direction);

    QByteArray senseBuffer() const;
    struct request_sense *sense();

    void reset(const int cmd_length);

private:
    int device_handle;

    QVector<quint8> cmd;
    QByteArray sense_buffer;
};

QByteArray mmcInquiry(const int deviceHandle, ErrorCode &errcode);

QByteArray modeSense(const int deviceHandle, ErrorCode &errcode, const int page);

/* TOC/PMA/ATIP:
 * format == 0: TOC
 * format == 1: Session information
 * format == 2: Full TOC
 * format == 3: PMA
 * format == 4: ATIP
 * format == 5: CD-Text */
QByteArray readTocPmaAtip(const int deviceHandle, ErrorCode &errcode, const quint8 format = 2, const bool msf = true, const int tracknumber = 1);

/* subchannelDataFormat:
 * 0x01: CD current position
 * 0x02: Media Catalog number (UPC/bar code)
 * 0x03: ISRC
 * tracknumber only valid if subchannelDataFormat == 0x03 */
QByteArray readSubchannel(const int deviceHandle, ErrorCode &errcode, const unsigned int subchannelDataFormat, const int tracknumber);

/* c2_err_ptrs:
 * 0x00: No error info
 * 0x01: 294 bytes, one bit for every byte of the 2352 bytes
 * 0x02: 296 bytes, xor of all c2 bits, zero pad bit, 294 c2 bits
 *
 * subchannelData
 * 0x00: No Sub-channel data
 * 0x01: RAW P-W Sub-channel (96 bytes)
 * 0x02: Formatted Q Sub-channel (16 bytes)
 * 0x04: Corrected and de-interleaved R-W Sub-channel (96 bytes) */
QByteArray
readCD(const int deviceHandle, ErrorCode &errcode, const int firstSector, const int len, const int c2_err_ptrs = 0x00, const int subchannelData = 0x00);

QString errorString(const int cmd, const ErrorCode errcode);

quint8 fromBCD8(const quint8 value);

}

}

}
