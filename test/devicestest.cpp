/*
  Copyright (C) 2016 Leslie Zhai <xiang.zhai@i-soft.com.cn>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

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
