/*
 * AUDEX CDDA EXTRACTOR
 *
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "cddadevices.h"

#include <QDebug>

CDDADevices::CDDADevices(QObject *parent)
{
    Q_UNUSED(parent);

    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString &)), this, SLOT(p_solid_device_added(const QString &)));
    connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString &)), this, SLOT(p_solid_device_removed(const QString &)));
}

CDDADevices::~CDDADevices()
{
    p_clear();
}

const QString CDDADevices::blockDevice(const QString &udi) const
{
    OpticalAudioDisc *disc = p_discs.value(udi, NULL);
    if (!disc)
        return QString();

    Solid::Device device(disc->device.parentUdi());
    if (device.is<Solid::Block>())
        return device.as<Solid::Block>()->device();

    return QString();
}

int CDDADevices::discCount() const
{
    return p_discs.count();
}

void CDDADevices::scanBus()
{
    p_clear();

    QList<Solid::Device> list = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDisc, QString());

    for (int i = 0; i < list.count(); ++i)
        p_solid_device_added(list.value(i).udi());
}

void CDDADevices::eject(const QString &udi)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << udi;
    OpticalAudioDisc *disc = p_discs.value(udi, NULL);
    if (!disc)
        return;

    Solid::Device device(disc->device.parentUdi());
    if (device.as<Solid::OpticalDrive>())
        device.as<Solid::OpticalDrive>()->eject();
}

void CDDADevices::setName(const QString &udi, const QString &name)
{
    OpticalAudioDisc *disc = p_discs.value(udi, NULL);
    if (!disc)
        return;

    if (disc->name != name)
        disc->name = name;
}

void CDDADevices::p_solid_device_added(const QString &udi)
{
    Solid::Device device(udi);

    qDebug() << "Device found:" << device.udi();
    if (p_is_optical_audio_disc(device)) {
        qDebug() << "is audio.";
        auto *disc = new OpticalAudioDisc();
        disc->name = i18n("Audio Disc");
        disc->device = device;
        p_discs.insert(udi, disc);
        Q_EMIT audioDiscDetected(udi);
    }
}

void CDDADevices::p_solid_device_removed(const QString &udi)
{
    OpticalAudioDisc *disc = p_discs.value(udi, NULL);

    if (disc) {
        qDebug() << "Optical audio disc removed:" << udi;
        delete disc;
        p_discs.remove(udi);
        Q_EMIT audioDiscRemoved(udi);
    }
}

bool CDDADevices::p_is_optical_audio_disc(const Solid::Device &device) const
{
    if (device.is<Solid::OpticalDisc>())
        return (device.as<Solid::OpticalDisc>()->availableContent() & Solid::OpticalDisc::Audio);

    return false;
}

void CDDADevices::p_clear()
{
    QHash<QString, OpticalAudioDisc *>::const_iterator i = p_discs.constBegin();
    while (i != p_discs.constEnd()) {
        if (i.value())
            delete i.value();
        ++i;
    }
    p_discs.clear();
}
