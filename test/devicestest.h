/*
  SPDX-FileCopyrightText: 2016 Leslie Zhai <xiang.zhai@i-soft.com.cn>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DEVICESTEST_H
#define DEVICESTEST_H

#include <QObject>

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
    QList<OpticalAudioDisc *> m_discs;
};

#endif
