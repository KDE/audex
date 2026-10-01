/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "scsidevice.h"

#include <QElapsedTimer>

#include <cerrno>
#include <cstring>

extern "C" {
#include <fcntl.h>
#ifdef Q_OS_LINUX
#include <scsi/sg.h>
#endif
#include <sys/ioctl.h>
#include <unistd.h>
}

using namespace Qt::StringLiterals;

namespace Audex::Scsi
{

namespace
{
constexpr quint16 DriverStatusMask = 0x0F;
constexpr quint16 DriverSense = 0x08; // sense buffer valid
constexpr quint16 DriverTimeout = 0x06;
constexpr quint16 HostTimeout = 0x03; // DID_TIME_OUT
constexpr quint8 StatusCheckCondition = 0x02;
}

bool Result::transportOk() const
{
    const quint16 driver = driverStatus & DriverStatusMask;
    return transportErrno == 0 && hostStatus == 0 && (driver == 0 || driver == DriverSense);
}

bool Result::ok() const
{
    return transportOk() && status == 0;
}

bool Result::recovered() const
{
    return transportOk() && status == StatusCheckCondition && sense.valid && sense.key == 0x01;
}

bool Result::timedOut() const
{
    return transportErrno == ETIMEDOUT || hostStatus == HostTimeout || (driverStatus & DriverStatusMask) == DriverTimeout;
}

QString Result::toString() const
{
    if (transportErrno != 0)
        return u"transport error: %1"_s.arg(QString::fromLocal8Bit(std::strerror(transportErrno)));
    if (hostStatus != 0)
        return u"host adapter error 0x%1%2"_s.arg(hostStatus, 2, 16, QLatin1Char('0')).arg(hostStatus == 0x03 ? u" (timeout)"_s : QString());
    if (!transportOk())
        return u"driver error 0x%1"_s.arg(driverStatus, 2, 16, QLatin1Char('0'));
    if (status == 0)
        return u"OK"_s;
    if (status == StatusCheckCondition)
        return u"CHECK CONDITION: %1"_s.arg(sense.toString());
    return u"SCSI status 0x%1"_s.arg(status, 2, 16, QLatin1Char('0'));
}

// ---------------------------------------------------------------------------

Device::~Device()
{
    close();
}

bool Device::open(const QString &path, QString *error)
{
    close();
#ifndef Q_OS_LINUX
    Q_UNUSED(path);
    if (error)
        *error = u"Direct disc access is not supported on this platform yet."_s;
    return false;
#else
    const QByteArray native = path.toLocal8Bit();

    m_fd = ::open(native.constData(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    m_writable = m_fd >= 0;
    if (m_fd < 0)
        m_fd = ::open(native.constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (m_fd < 0) {
        if (error)
            *error = u"Unable to open %1: %2"_s.arg(path, QString::fromLocal8Bit(std::strerror(errno)));
        return false;
    }

    int version = 0;
    if (::ioctl(m_fd, SG_GET_VERSION_NUM, &version) < 0) {
        if (error)
            *error = u"%1 does not support SG_IO"_s.arg(path);
        close();
        m_timeouts = 0; // counted per session (the count stays readable after close())
        return false;
    }

    m_path = path;
    return true;
#endif
}

void Device::close()
{
    if (m_fd >= 0)
        ::close(m_fd);
    m_fd = -1;
    m_writable = false;
}

Result Device::execute(QByteArrayView cdb, char *data, int length, Direction direction, int timeoutMs)
{
    Result result;
    if (m_fd < 0) {
        result.transportErrno = EBADF;
        return result;
    }

#ifdef Q_OS_LINUX
    unsigned char cmd[16] = {};
    const auto cmdLength = std::min<qsizetype>(cdb.size(), sizeof(cmd));
    std::memcpy(cmd, cdb.constData(), cmdLength);

    unsigned char sense[64] = {};

    sg_io_hdr_t hdr;
    std::memset(&hdr, 0, sizeof(hdr));
    hdr.interface_id = 'S';
    hdr.cmd_len = static_cast<unsigned char>(cmdLength);
    hdr.cmdp = cmd;
    hdr.mx_sb_len = sizeof(sense);
    hdr.sbp = sense;
    hdr.timeout = static_cast<unsigned int>(timeoutMs);
    hdr.dxferp = data;
    hdr.dxfer_len = data ? static_cast<unsigned int>(length) : 0;
    switch (direction) {
    case Direction::None:
        hdr.dxfer_direction = SG_DXFER_NONE;
        hdr.dxfer_len = 0;
        hdr.dxferp = nullptr;
        break;
    case Direction::FromDevice:
        hdr.dxfer_direction = SG_DXFER_FROM_DEV;
        break;
    case Direction::ToDevice:
        hdr.dxfer_direction = SG_DXFER_TO_DEV;
        break;
    }

    QElapsedTimer timer;
    timer.start();
    const int rc = ::ioctl(m_fd, SG_IO, &hdr);
    result.elapsedUs = timer.nsecsElapsed() / 1000;

    if (rc < 0) {
        result.transportErrno = errno;
        if (result.timedOut())
            ++m_timeouts;
        return result;
    }

    result.status = hdr.status;
    result.hostStatus = hdr.host_status;
    result.driverStatus = hdr.driver_status;
    result.transferred = int(hdr.dxfer_len) - hdr.resid;
    result.sense = Mmc::parseSense(QByteArrayView(reinterpret_cast<const char *>(sense), std::min<int>(hdr.sb_len_wr, sizeof(sense))));
    if (result.timedOut())
        ++m_timeouts;
    return result;
#else
    Q_UNUSED(cdb);
    Q_UNUSED(data);
    Q_UNUSED(length);
    Q_UNUSED(direction);
    Q_UNUSED(timeoutMs);
    result.transportErrno = ENOTSUP;
    return result;
#endif
}

}
