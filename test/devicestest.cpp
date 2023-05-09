// SPDX-FileCopyrightText: 2016 Leslie Zhai <xiang.zhai@i-soft.com.cn>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "devicestest.h"

#include <QTest>
#include <QtDebug>

#include <Solid/OpticalDrive>

void DevicesTest::initTestCase()
{
    // Purposely use OpticalDrive
    QList<Solid::Device> list = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDrive, QString());
    for (int i = 0; i < list.count(); i++) {
        const QString udi = list.value(i).udi();
        Solid::Device device(udi);
        qDebug() << "Device found:" << device.udi();
        auto *disc = new OpticalAudioDisc;
        disc->name = udi;
        disc->device = device;
        m_discs << disc;
    }
}

void DevicesTest::cleanupTestCase()
{
}

void DevicesTest::testEject()
{
    if (m_discs.isEmpty())
        return;

    OpticalAudioDisc *disc = m_discs[0];
    if (!disc)
        return;

    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << disc->name;

    // Purposely use OpticalDrive's parent
    Solid::Device device(disc->device.parentUdi());
    if (device.as<Solid::OpticalDrive>())
        device.as<Solid::OpticalDrive>()->eject();
}

QTEST_GUILESS_MAIN(DevicesTest)
