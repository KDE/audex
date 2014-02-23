/*
 * AUDEX CDDA EXTRACTOR
 *
 * Copyright (C) 2007-2014 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CDDASOLID_H
#define CDDASOLID_H

#include <QAbstractTableModel>
#include <QString>
#include <QStringList>

#include <KIcon>
#include <KDebug>
#include <KLocale>

#include <Solid/Device>
#include <Solid/StorageAccess>
#include <Solid/OpticalDrive>
#include <Solid/OpticalDisc>
#include <Solid/DeviceNotifier>
#include <Solid/DeviceInterface>
#include <Solid/Block>
#include <Solid/GenericInterface>

#include <utils/error.h>

struct OpticalAudioDisc {
  QString name;
  Solid::Device device;
};

class CDDASolid : public QObject {

  Q_OBJECT

public:
  CDDASolid(QObject *parent = 0);
  ~CDDASolid();

  const QString blockDevice(const QString& udi) const;

  int discCount() const;

public slots:
  void scanBus();

  void requestEject(const QString& udi);

  //set display name
  void setName(const QString& udi, const QString& name);

signals:
  void audioDiscDetected(const QString& udi);
  void audioDiscRemoved(const QString& udi);

  void error(const Error& error);

private slots:
  void p_solid_device_added(const QString& udi);
  void p_solid_device_removed(const QString& udi);

private:
  QHash<QString, OpticalAudioDisc*> p_discs;

  bool p_is_optical_audio_disc_in_drive(const Solid::Device& device) const;

  void p_clear();

};

#endif
