/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "driveinforetriever.h"

#include "scsi.h"

namespace Audex
{

namespace Device
{

const DriveInfo retrieveDriveInfo(const QString &driveUDI, QString &error)
{
    Solid::Device device(driveUDI);
    const QByteArray block_device = device.as<Solid::Block>()->device().toLatin1();

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << block_device;

    DriveInfo drive_info;

    drive_info.set(DriveInfo::UDI, driveUDI);
    drive_info.set(DriveInfo::BlockDevice, block_device);

    int deviceHandle = ::open(block_device.constData(), O_RDONLY | O_NONBLOCK);
    if (deviceHandle == -1) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to initialize device:" << block_device;
        error = i18n("Failed to initialize drive %1.", QString::fromLatin1(block_device));
        return DriveInfo();
    }

    SCSI::ErrorCode ec;
    QByteArray buffer = SCSI::mmcInquiry(deviceHandle, ec);

    if (ec) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to inquiry drive:" << block_device;
        error = i18n("Failed to inquiry drive %1: %2", QString::fromLatin1(block_device)), ec.senseKeyString();
        ::close(deviceHandle);
        return DriveInfo();
    }

    struct inquiry *inq = (struct inquiry *)buffer.data();

    drive_info.set(DriveInfo::Vendor, QString::fromLatin1(QByteArray(reinterpret_cast<char *>(inq->vendor), 8).trimmed()));
    drive_info.set(DriveInfo::Model, QString::fromLatin1(QByteArray(reinterpret_cast<char *>(inq->model), 16).trimmed()));
    drive_info.set(DriveInfo::Revision, QString::fromLatin1(QByteArray(reinterpret_cast<char *>(inq->revision), 4).trimmed()));

    buffer = SCSI::modeSense(deviceHandle, ec, 0x2A);

    if (ec) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to detect drive capabilities for drive:" << block_device;
        error = i18n("Failed to detect drive capabilities for drive %1: %2", QString::fromLatin1(block_device)), ec.senseKeyString();
        ::close(deviceHandle);
        return DriveInfo();
    }

    mm_cap_page_2A *mm_p = (mm_cap_page_2A *)(buffer.data() + 8);

    DriveInfo::CapabilitySet capabilities;

    if (mm_p->c2_pointers)
        capabilities.insert(DriveInfo::C2_ERR_PTRS);
    if (mm_p->UPC)
        capabilities.insert(DriveInfo::READ_MCN);
    if (mm_p->ISRC)
        capabilities.insert(DriveInfo::READ_ISRC);
    if (mm_p->cd_da_supported)
        capabilities.insert(DriveInfo::READ_CDDA);
    if (mm_p->cd_da_accurate)
        capabilities.insert(DriveInfo::READ_CDDA_ACCURATE);

    drive_info.setCapabilities(capabilities);

    ::close(deviceHandle);

    return drive_info;
}

}

}
