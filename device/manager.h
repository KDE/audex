/*
 * AUDEX CDDA EXTRACTOR
 *
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef MANAGER_H
#define MANAGER_H

#include <QDebug>
#include <QInputDialog>
#include <QList>
#include <QMap>
#include <QString>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>

#include <KLocalizedString>

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/DeviceNotifier>
#include <Solid/GenericInterface>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/StorageAccess>

#include <KCDDB/CDInfo>
#include <KCDDB/Client>
#include <KCDDB/KCDDB>

#include "datatypes/message.h"

#include "drive.h"
#include "cdtext_reader_task.h"
#include "driveinfo_fetcher_task.h"
#include "mcnisrc_reader_task.h"
#include "rip_task.h"
#include "toc_reader_task.h"

namespace Audex
{

namespace Device
{

class Manager : public QObject
{
    Q_OBJECT

public:
    explicit Manager(QObject *parent = nullptr);
    ~Manager() override;

    Message lastError() const;
    void resetError();

    void enableAutoReadTOC(const bool enabled = true);
    bool autoReadTOCEnabled() const;

    void enableAccurateRipVerify(const bool enabled = true);
    bool accurateRipVerify() const;

    int driveCount() const;
    QList<QString> driveUDIS() const;
    Drive *drive(const QString &udi) const;

    DriveList driveList() const;

    CDInfo getCDInfoByDrive(const QString &driveUDI) const;
    bool discInDriveAvailable(const QString &driveUDI) const;

public Q_SLOTS:
    void scanBus();

    void eject(const QString &driveUDI) const;

    void startFetchDriveInfoTask(const QString &driveUDI, const bool blocking = false);
    void startReadTOCTask(const QString &driveUDI, const bool blocking = false);
    void startReadCDTextTask(const QString &driveUDI, const bool blocking = false);
    void startReadMcnIsrcTask(const QString &driveUDI, const bool readMCN = true, const TracknumberSet &tracks = TracknumberSet(), const bool blocking = false);
    void startRipTask(const QString &driveUDI, const TracknumberSet &tracks); // always non blocking
    void stopTask(const QString &driveUDI);

    void lookupCDDB(const QString &driveUDI, const bool blocking = false);

    void verifyAccurateRip(const QString &driveUDI, const Checksum32Map checksums, const Checksum32Map checksumsV2);

Q_SIGNALS:
    void driveDetected(const QString &driveUDI, const Drive *drive);
    void driveRemoved(const QString &driveUDI);

    void audioDiscDetected(const QString &driveUDI, const QString &discUDI);
    void audioDiscRemoved(const QString &driveUDI, const QString &discUDI);

    void taskStarted(const QString &driveUDI);
    void taskFinished(const QString &driveUDI);

    void fetchDriveInfoTaskFinished(const QString &driveUDI, const bool successful, const DriveInfo &driveInfo);
    void readTOCTaskFinished(const QString &driveUDI, const bool successful, const Toc::Toc &toc);
    void readCDTextTaskFinished(const QString &driveUDI, const bool successful, const Metadata::Metadata &metadata);
    void readMcnIsrcTaskFinished(const QString &driveUDI, const bool successful, const Metadata::Metadata &metadata);

    void CDDBLookupFinished(const QString &driveUDI, const bool successful, const Metadata::Metadata &metadata = Metadata::Metadata());

    void ripTaskOutput(const QString &driveUDI, const QByteArray &);
    void ripTaskProgress(const QString &driveUDI,
                         const int tracknumber,
                         const qreal fractionCurrentTrack,
                         const qreal fraction,
                         const int currentSector,
                         const int sectorsRead,
                         const qreal currentSpeed);
    void ripTaskNextTrack(const QString &driveUDI, const int prev_tracknumber, const int tracknumber);
    void ripTaskFinished(const QString &driveUDI, const bool successful);

    void log(const QString &driveUDI, const Message &msg);

private Q_SLOTS:
    void add_device(const QString &drive_udi);
    void remove_device(const QString &drive_udi);

    void fetch_drive_info_task_finished(const QString &drive_udi, const bool successful, const DriveInfo &);
    void read_toc_task_finished(const QString &drive_udi, const bool successful, const Toc::Toc &);
    void read_cdtext_task_finished(const QString &drive_udi, const bool successful, const Metadata::Metadata &);
    void read_mcnisrc_task_finished(const QString &drive_udi, const bool successful, const Metadata::Metadata &);

    void cddb_lookup_finished(KCDDB::Result result);

    void rip_task_output(const QString &drive_udi, const QByteArray &);
    void rip_task_progress(const QString &drive_udi, const int tracknumber, const qreal fractionCurrentTrack, const qreal fraction, const int currentSector, const int sectorsRead, const qreal currentSpeed);
    void rip_task_nexttrack(const QString &drive_udi, const int prev_tracknumber, const int tracknumber);
    void rip_task_finished(const QString &drive_udi, const bool successful, const Checksum32Map &checksumsCRC32, const Checksum32Map &checksumsACR, const Checksum32Map &checksumsACRV2);

    void task_log(const QString &drive_udi, const Message &msg);

    void accuraterip_database_lookup_finished(const bool successful, const QByteArray &id, const AccurateRip::Records &records);

private:
    DriveMap drives; // Attached drives per UDI
    QMap<QString, Task *> tasks; // tasks per drive by drive UDI
    QMutex tasks_mutex;

    bool has_task(const QString &drive_udi);
    void register_task(const QString &drive_udi, Task *task);
    void unregister_task(Task *task);

    bool is_drive_udi(const QString &udi) const;
    Drive *get_drive_by_disc_udi(const QString &disc_udi) const;
    bool is_disc_udi(const QString &udi) const;

    bool auto_read_toc;
    bool accuraterip_verfiy;

    void add_drive(const QString &drive_udi);
    void add_disc(const QString &disc_udi);

    KCDDB::Client cddb;
    QString cddb_driveudi_cache;

    AccurateRip::Database acr_database;
    QString acr_driveudi_cache;
    Checksum32Map acr_checksums_cache;
    Checksum32Map acr_checksums_v2_cache;

    Message last_error;
};

}

}

#endif
