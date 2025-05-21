/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// This code is inspired by K3B, 1998-2009 Sebastian Trueg <trueg@k3b.org>

#include "scsi.h"

namespace Audex
{

namespace Device
{

namespace SCSI
{

QString ErrorCode::senseKeyString() const
{
    switch (p_sense_key) {
    case 0x0:
        return i18n("No sense (0)");
    case 0x1:
        return i18n("Recover error (1)");
    case 0x2:
        return i18n("Logical unit not ready (2)");
    case 0x3:
        return i18n("Medium error (3)");
    case 0x4:
        return i18n("Hardware error (4)");
    case 0x5:
        return i18n("Illegal request (5)");
    case 0x6:
        return i18n("Unit attention (6)");
    case 0x7:
        return i18n("Data protect (7)");
    case 0x8:
        return i18n("Blank check (8)");
    case 0x9:
        return i18n("Vendor specific (9)");
    case 0xA:
        return i18n("Copy aborted (A)");
    case 0xB:
        return i18n("Aborted command (B)");
    }
    return i18n("Unknown error (C)");
}

QDebug operator<<(QDebug debug, const ErrorCode &errcode)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "(Error Code=" << errcode.errorCode() << ", Sense Key=" << errcode.senseKey() << ", ASC=" << errcode.asc() << ", ASCQ=" << errcode.ascq()
                    << ", Error String=" << errcode.senseKeyString() << ")\n";
    return debug;
}

ScsiCommand::ScsiCommand(const int deviceHandle, const int cmdLength)
{
    device_handle = deviceHandle;
    reset(cmdLength);
}

quint8 &ScsiCommand::operator[](const int i)
{
    return cmd[i];
}

int ScsiCommand::send(QByteArray *buffer, const int direction)
{
    sg_io_hdr_t io_hdr;
    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));

    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = cmd.length();
    io_hdr.flags = SG_FLAG_LUN_INHIBIT | SG_FLAG_DIRECT_IO;
    io_hdr.mx_sb_len = 32;
    io_hdr.dxfer_direction = direction;
    io_hdr.dxfer_len = buffer->size();
    io_hdr.dxferp = buffer->data();
    io_hdr.cmdp = cmd.data();
    io_hdr.sbp = reinterpret_cast<unsigned char *>(sense_buffer.data());
    io_hdr.timeout = 10000;

    int error = ::ioctl(device_handle, SG_IO, &io_hdr);

    if (error < 0)
        error = errno;
    else if (io_hdr.status != 0)
        error = io_hdr.status;
    else if (io_hdr.host_status != 0)
        error = io_hdr.host_status;
    else if ((io_hdr.info & SG_INFO_OK_MASK) != SG_INFO_OK)
        error = io_hdr.info & SG_INFO_OK_MASK;

    return error;
}

QByteArray ScsiCommand::senseBuffer() const
{
    return sense_buffer;
}

struct request_sense *ScsiCommand::sense()
{
    return reinterpret_cast<struct request_sense *>(sense_buffer.data());
}

void ScsiCommand::reset(const int cmd_length)
{
    cmd = QVector<quint8>(cmd_length, 0);
    sense_buffer = QByteArray(MAX_SENSEBUFFER_LENGTH, 0);
}

QByteArray mmcInquiry(const int deviceHandle, ErrorCode &errcode)
{
    ScsiCommand cmd(deviceHandle, 10);

    errcode.clear();

    quint16 buffer_len = 36;
    QByteArray buffer(buffer_len, 0);

    cmd[0] = MMC_INQUIRY;
    cmd[4] = buffer_len;
    cmd[5] = 0;

    int error = cmd.send(&buffer, SG_DXFER_FROM_DEV);

    if (error) {
        errcode = ErrorCode(cmd.sense()->error_code, cmd.sense()->sense_key, cmd.sense()->asc, cmd.sense()->ascq);
        return QByteArray();
    }

    return buffer;
}

QByteArray modeSense(const int deviceHandle, ErrorCode &errcode, const int page)
{
    ScsiCommand cmd(deviceHandle, 10);

    errcode.clear();

    quint16 header_len = 2048;
    QByteArray header(header_len, 0);

    cmd[0] = MMC_MODE_SENSE;
    cmd[1] = 0x8; // Disable Block Descriptors
    cmd[2] = page & 0x3F;
    cmd[8] = 8;
    cmd[9] = 0; // Necessary to set the proper command length

    quint16 buffer_len = 8;

    int error = cmd.send(&header, SG_DXFER_FROM_DEV);

    if (error) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "MODE SENSE length detection failed. Taking default value.";
    } else {
        buffer_len = qFromBigEndian(*reinterpret_cast<const quint16 *>(header.constData())) + 2;
    }

    // FIXME: rumor or misnomer?
    // Some buggy firmwares do not return the size of the available data
    // but the returned data. So we simply use the maximum possible value to be
    // on the safe side with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high
    // data length.
    if (buffer_len == 8)
        buffer_len = 0xFFFF;

    QByteArray buffer(buffer_len, 0);

    cmd[7] = buffer_len >> 8;
    cmd[8] = buffer_len;

    error = cmd.send(&buffer, SG_DXFER_FROM_DEV);

    if (error) {
        errcode = ErrorCode(cmd.sense()->error_code, cmd.sense()->sense_key, cmd.sense()->asc, cmd.sense()->ascq);
        return QByteArray();
    }

    return buffer;
}

QByteArray readTocPmaAtip(const int deviceHandle, ErrorCode &errcode, const quint8 format, const bool msf, const int tracknumber)
{
    ScsiCommand cmd(deviceHandle, 10);

    errcode.clear();

    unsigned int desc_len;
    switch (format) {
    case 0:
        desc_len = 8;
        break;
    case 1:
        desc_len = 8;
        break;
    case 2:
        desc_len = 11;
        break;
    case 3:
        desc_len = 11;
        break;
    case 4:
        desc_len = 4; // MMC2: 24 and MMC4: 28, so we use the highest common factor
        break;
    case 5:
        desc_len = 18;
    }

    quint16 header_len = 4;

    cmd[0] = MMC_READ_TOC_PMA_ATIP;
    if (msf)
        cmd[1] = 0x02;
    cmd[2] = format & 0x0F;
    cmd[6] = tracknumber;
    cmd[8] = 4;

    if ((format & 0xF) == 5)
        header_len = 32768;

    QByteArray header(header_len, 0);

    int error = cmd.send(&header, SG_DXFER_FROM_DEV);

    if (error) {
        errcode = ErrorCode(cmd.sense()->error_code, cmd.sense()->sense_key, cmd.sense()->asc, cmd.sense()->ascq);
        return QByteArray();
    }

    // now take the real length
    quint16 buffer_len = qFromBigEndian(*reinterpret_cast<const quint16 *>(header.constData())) + 2;
    QByteArray buffer(buffer_len, 0);

    // Some buggy firmwares return an invalid size here
    // So we simply use the maximum possible value to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    if (desc_len && ((buffer_len - 4) % desc_len || buffer_len < 4 + desc_len)) {
        buffer_len = 0xFFFF;
    }

    // Not all drives like uneven numbers
    if (buffer_len % 2)
        ++buffer_len;

    cmd[7] = buffer_len >> 8;
    cmd[8] = buffer_len;

    error = cmd.send(&buffer, SG_DXFER_FROM_DEV);

    if (error) {
        errcode = ErrorCode(cmd.sense()->error_code, cmd.sense()->sense_key, cmd.sense()->asc, cmd.sense()->ascq);
        return QByteArray();
    }

    // buffer.resize(qMin(buffer.size(), qFromBigEndian(*reinterpret_cast<const quint16 *>(buffer.constData())) + 2));

    return buffer;
}

QByteArray readSubchannel(const int deviceHandle, ErrorCode &errcode, const unsigned int subchannelDataFormat, const int tracknumber)
{
    ScsiCommand cmd(deviceHandle, 10);

    errcode.clear();

    cmd[0] = MMC_READ_SUB_CHANNEL;
    cmd[2] = 0x40; // SUBQ
    cmd[3] = subchannelDataFormat;
    if (subchannelDataFormat == 0x03)
        cmd[6] = tracknumber; // only used when subchannelDataFormat == 03h (ISRC)
    cmd[8] = 4;

    QByteArray header(4, 0);

    int error = cmd.send(&header, SG_DXFER_FROM_DEV);

    if (error) {
        errcode = ErrorCode(cmd.sense()->error_code, cmd.sense()->sense_key, cmd.sense()->asc, cmd.sense()->ascq);
        return QByteArray();
    }

    // now take the real length
    quint16 buffer_len = qFromBigEndian(*reinterpret_cast<const quint16 *>(&header.constData()[2])) + 4;

    // Some buggy firmwares do not return the size of the available data
    // but the returned data. So we simply use the maximum possible value to be on the safe side
    // with these buggy drives.
    // We cannot use this as default since many firmwares fail with a too high data length.
    if (buffer_len <= 4)
        buffer_len = 0xFFFF;

    QByteArray buffer(buffer_len, 0);

    cmd[7] = buffer_len >> 8;
    cmd[8] = buffer_len;

    error = cmd.send(&buffer, SG_DXFER_FROM_DEV);

    if (error) {
        errcode = ErrorCode(cmd.sense()->error_code, cmd.sense()->sense_key, cmd.sense()->asc, cmd.sense()->ascq);
        return QByteArray();
    }

    // buffer.resize(qMin(buffer.size(), qFromBigEndian(*reinterpret_cast<const quint16 *>(&buffer.constData()[2])) + 4));

    return buffer;
}

QByteArray readCD(const int deviceHandle, ErrorCode &errcode, const int firstSector, const int len, const int c2_err_ptrs, const int subchannelData)
{
    ScsiCommand cmd(deviceHandle, 12);

    errcode.clear();

    int buffer_size = len * SECTOR_SIZE_BYTES;

    if (subchannelData == 0x01)
        buffer_size += len * 96;
    else if (subchannelData == 0x02)
        buffer_size += len * FORMATED_Q_SUBCHANNEL_SIZE_BYTES;
    else if (subchannelData == 0x04)
        buffer_size += len * CORRECTED_PW_SUBCHANNEL_SIZE_BYTES;

    if (c2_err_ptrs == 0x01)
        buffer_size += len * C2_SIZE_BYTES;
    else if (c2_err_ptrs == 0x02)
        buffer_size += len * (C2_SIZE_BYTES + 2);

    int sector_type = 0x01; // CD-DA
    bool dap = false;
    bool sync = true;
    bool header = true;
    bool subHeader = true;
    bool userData = true;
    bool edcEcc = true;

    QByteArray buffer(buffer_size, 0);

    cmd[0] = MMC_READ_CD;
    cmd[1] = (sector_type << 2 & 0x1c) | (dap ? 0x2 : 0x0);
    cmd[2] = firstSector >> 24;
    cmd[3] = firstSector >> 16;
    cmd[4] = firstSector >> 8;
    cmd[5] = firstSector;
    cmd[6] = len >> 16;
    cmd[7] = len >> 8;
    cmd[8] = len;
    cmd[9] =
        ((sync ? 0x80 : 0x0) | (subHeader ? 0x40 : 0x0) | (header ? 0x20 : 0x0) | (userData ? 0x10 : 0x0) | (edcEcc ? 0x8 : 0x0) | (c2_err_ptrs << 1 & 0x6));
    cmd[10] = subchannelData & 0x7;

    int error = cmd.send(&buffer, SG_DXFER_FROM_DEV);

    if (error) {
        errcode = ErrorCode(cmd.sense()->error_code, cmd.sense()->sense_key, cmd.sense()->asc, cmd.sense()->ascq);
        return QByteArray();
    }

    return buffer;
}

quint8 fromBCD8(const quint8 value)
{
    return (0xf & value) + (10 * (value >> 4));
}

}

}

}
