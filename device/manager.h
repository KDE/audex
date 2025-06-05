/*
 * AUDEX CDDA EXTRACTOR
 *
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <QDebug>
#include <QInputDialog>
#include <QMap>
#include <QSharedPointer>
#include <QString>

#include <KLocalizedString>

#include <KCDDB/CDInfo>
#include <KCDDB/Client>
#include <KCDDB/KCDDB>

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/DeviceNotifier>
#include <Solid/GenericInterface>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/StorageAccess>

#include "cdtextreader.h"
#include "datatypes/cdda.h"

namespace Audex
{

namespace Device
{

struct OpticalAudioDisc;

struct OpticalDiscDrive {
    Solid::Device device;
    QString vendor;
    QString model; // == product
};

typedef QMap<QString, QSharedPointer<OpticalDiscDrive>> OpticalDiscDrives;

struct OpticalAudioDisc {
    Solid::Device device;
    CDDA cdda;
    QSharedPointer<OpticalDiscDrive> drive;
};

typedef QMap<QString, QSharedPointer<OpticalAudioDisc>> OpticalAudioDiscs;

class Manager : public QObject
{
    Q_OBJECT

public:
    explicit Manager(QObject *parent = nullptr);
    ~Manager() override;

    void enableAutoReadTOC(const bool enabled = true)
    {
        auto_read_toc = enabled;
    }
    bool autoReadTOCEnabled() const
    {
        return auto_read_toc;
    }

    const QString currentDisc() const
    {
        return current_disc_udi;
    }
    bool currentDiscAvailable() const
    {
        return !current_disc_udi.isEmpty();
    }
    const CDDA currentCDDA() const
    {
        return cdda(current_disc_udi);
    }

    const QStringList driveUDIList() const;
    const QString driveVendor(const QString &driveUDI) const;
    const QString driveModel(const QString &driveUDI) const;
    const QByteArray blockDevice(const QString &driveUDI) const;

    const QStringList discUDIList() const;
    const CDDA cdda(const QString &discUDI) const;
    const QByteArray blockDeviceFromDisc(const QString &discUDI) const;

    const QString discPrettyName(const QString &discUDI) const;

public Q_SLOTS:
    void setCurrentDisc(const QString &udi)
    {
        current_disc_udi = udi;
    }
    void clearCurrentDisc()
    {
        current_disc_udi.clear();
    }

    void scanBus();

    void readTOC(const QString &discUDI);

    void eject(const QString &driveUDI);
    void ejectDisc(const QString &discUDI);

    void lookupCDDB(const QString &discUDI, const bool blocking = false);

    void readCDText(const QString &discUDI);

    void setMetadata(const QString &discUDI, const Metadata::Metadata metadata);
    void setMetadataInCurrentDisc(const Metadata::Metadata metadata);

Q_SIGNALS:
    void driveDetected(const QString &driveUDI, const QString &vendor, const QString &model);
    void driveRemoved(const QString &driveUDI);

    void audioDiscDetected(const QString &driveUDI, const QString &discUDI, const CDDA &cdda);
    void audioDiscRemoved(const QString &discUDI);

    void CDDBLookupDone(const QString &discUDI, const bool successful, const Metadata::Metadata &metadata = Metadata::Metadata());
    void CDTextReadDone(const QString &discUDI, const bool successful, const Metadata::Metadata &metadata = Metadata::Metadata());

private Q_SLOTS:
    void add_device(const QString &udi);
    void remove_device(const QString &udi);

    void cddb_lookup_done(KCDDB::Result result);
    void cdtext_lookup_done(const QString &discUDI, const bool successful, const Metadata::Metadata metadata);

private:
    QString current_disc_udi;

    OpticalDiscDrives p_drives;
    OpticalAudioDiscs p_audio_discs;

    bool auto_read_toc;

    KCDDB::Client cddb;
    QString cddb_discudi_cache;

    CDTextReader cd_text;

    void add_drive(const QString &drive_udi);
    void add_disc(const QString &disc_udi);

    bool is_optical_audio_disc(const Solid::Device &device) const;

    void clear();
};

}

}
