/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "core/mmcparse.h"

#include <QByteArrayView>
#include <QString>

namespace Audex::Scsi
{

enum class Direction {
    None,
    FromDevice,
    ToDevice
};

// Outcome of one SCSI command. Transport problems (ioctl failure, timeout,
// host adapter errors) are reported separately from SCSI status and sense data,
// so an empty sense buffer can never be mistaken for success.
struct Result {
    int transportErrno = 0;
    quint8 status = 0; // SCSI status byte (0x00 GOOD, 0x02 CHECK CONDITION)
    quint16 hostStatus = 0;
    quint16 driverStatus = 0;
    Mmc::Sense sense;
    int transferred = 0; // dxfer_len - resid
    qint64 elapsedUs = 0;

    bool transportOk() const;
    bool ok() const; // GOOD status
    bool recovered() const; // CHECK CONDITION with RECOVERED ERROR: data is valid
    bool dataUsable() const
    {
        return ok() || recovered();
    }
    bool timedOut() const; // no answer in time: the kernel aborted the command
    QString toString() const;
};

// RAII wrapper around a Linux SG_IO capable device node (/dev/sr0, /dev/sg1).
class Device
{
public:
    Device() = default;
    ~Device();
    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;

    // Tries read/write first (needed e.g. for SET CD SPEED), falls back to read-only.
    bool open(const QString &path, QString *error = nullptr);
    void close();

    bool isOpen() const
    {
        return m_fd >= 0;
    }
    bool isWritable() const
    {
        return m_writable;
    }
    QString path() const
    {
        return m_path;
    }

    Result execute(QByteArrayView cdb, char *data, int length, Direction direction, int timeoutMs = 30000);

    // commands that timed out since the device was opened
    int timeouts() const
    {
        return m_timeouts;
    }

private:
    int m_fd = -1;
    bool m_writable = false;
    QString m_path;
    int m_timeouts = 0;
};

}
