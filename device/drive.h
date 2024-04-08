/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DRIVE_H
#define DRIVE_H

#include <QMutex>
#include <QList>
#include <QMap>
#include <QAtomicInteger>

#include <Solid/Block>
#include <Solid/Device>

#include "datatypes/cdinfo.h"
#include "datatypes/driveinfo.h"

namespace Audex
{

namespace Device
{

class Drive
{
public:
    Drive(const QString& udi = QString());

    QString getUDI() const {
        return udi;
    }
    QByteArray getBlockDevice() const {
        return block_device;
    }

    QMutex *driveInfoMutex()
    {
        return &drive_info_mutex;
    }
    const DriveInfo driveInfo() const
    {
        return drive_info;
    }
    DriveInfo &driveInfo()
    {
        return drive_info;
    }

    void setDriveInfo(const DriveInfo& driveInfo) {
        drive_info_mutex.lock();
        drive_info = driveInfo;
        drive_info_mutex.unlock();
    }

    QMutex *cdInfoMutex()
    {
        return &cd_info_mutex;
    }
    const CDInfo cdInfo() const
    {
        return cd_info;
    }
    CDInfo &cdInfo()
    {
        return cd_info;
    }

    void setCDInfo(const CDInfo& cdInfo) {
        cd_info_mutex.lock();
        cd_info = cdInfo;
        cd_info_mutex.unlock();
    }
    void clearCDInfo() {
        cd_info_mutex.lock();
        cd_info.clear();
        cd_info_mutex.unlock();
    }

    bool discAvailable()
    {
        cd_info_mutex.lock();
        bool result = !cd_info.isEmpty();
        cd_info_mutex.unlock();
        return result;
    }

protected:
    QString udi;
    QByteArray block_device;

    DriveInfo drive_info;
    QMutex drive_info_mutex;

    CDInfo cd_info;
    QMutex cd_info_mutex;
};

}

typedef QList<Device::Drive *> DriveList;
typedef QMap<QString, Device::Drive *> DriveMap;

}

#endif
