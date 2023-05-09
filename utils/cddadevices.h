/*
 * AUDEX CDDA EXTRACTOR
 *
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef CDDADEVICES_H
#define CDDADEVICES_H

#include <QAbstractTableModel>
#include <QHash>
#include <QString>
#include <QStringList>

#include <KLocalizedString>

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/DeviceNotifier>
#include <Solid/GenericInterface>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>
#include <Solid/StorageAccess>

struct OpticalAudioDisc {
    QString name;
    Solid::Device device;
};

class CDDADevices : public QObject
{
    Q_OBJECT

public:
    explicit CDDADevices(QObject *parent = nullptr);
    ~CDDADevices() override;

    const QString blockDevice(const QString &udi) const;

    const QStringList udiList() const;
    int discCount() const;

public Q_SLOTS:
    void scanBus();

    void eject(const QString &udi);

    // set display name
    void setName(const QString &udi, const QString &name);

Q_SIGNALS:
    void audioDiscDetected(const QString &udi);
    void audioDiscRemoved(const QString &udi);

private Q_SLOTS:
    void p_solid_device_added(const QString &udi);
    void p_solid_device_removed(const QString &udi);

private:
    QHash<QString, OpticalAudioDisc *> p_discs;

    bool p_is_optical_audio_disc(const Solid::Device &device) const;

    void p_clear();
};

#endif
