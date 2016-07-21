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

#ifndef DEVICESTEST_H
#define DEVICESTEST_H

#include <QtCore/QObject>

#include <Solid/Device>

struct OpticalAudioDisc {
    QString name;
    Solid::Device device;
};

class DevicesTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testEject();
private:
    QList<OpticalAudioDisc*> m_discs;
};

#endif
