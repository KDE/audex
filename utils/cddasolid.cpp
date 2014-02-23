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

#include "cddasolid.h"

CDDASolid::CDDASolid(QObject *parent) {

  Q_UNUSED(parent);

  connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString&)), this, SLOT(p_solid_device_added(const QString&)));
  connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString&)), this, SLOT(p_solid_device_removed(const QString&)));

}

CDDASolid::~CDDASolid() {

  p_clear();

}

const QString CDDASolid::blockDevice(const QString& udi) const {

  OpticalAudioDisc *disc = p_discs.value(udi, NULL);
  if (!disc) return QString();

  const Solid::GenericInterface *gen = disc->device.as<Solid::GenericInterface>();
  return gen->allProperties().value("DeviceFile").toString();

}

int CDDASolid::discCount() const {

  return p_discs.count();

}

void CDDASolid::scanBus() {

  p_clear();

  QList<Solid::Device> list = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDisc, QString());

  for (int i = 0; i < list.count(); ++i) {
    p_solid_device_added(list.value(i).udi());
  }

}

void CDDASolid::requestEject(const QString& udi) {

  OpticalAudioDisc *disc = p_discs.value(udi, NULL);
  if (!disc) return;

  Solid::Device device = disc->device;
  if (!p_is_optical_audio_disc_in_drive(device)) return;
  const Solid::GenericInterface *gen = device.as<Solid::GenericInterface>();

  QString driveUdi = gen->allProperties().value("block.storage_device").toString();
  Solid::Device device2(driveUdi);
  if (device2.is<Solid::OpticalDrive>()) {
    Solid::OpticalDrive *opticalDrive = device2.as<Solid::OpticalDrive>();
    opticalDrive->eject();
  }

}

void CDDASolid::setName(const QString& udi, const QString& name) {

  OpticalAudioDisc *disc = p_discs.value(udi, NULL);
  if (!disc) return;

  if (disc->name != name) {
    disc->name = name;
  }

}

void CDDASolid::p_solid_device_added(const QString& udi) {

  Solid::Device device(udi);

  kDebug() << "Device found:" << device.udi();
  if (p_is_optical_audio_disc_in_drive(device)) {
    kDebug() << "is audio.";
    OpticalAudioDisc *disc = new OpticalAudioDisc();
    disc->name = i18n("Audio Disc");
    disc->device = device;
    p_discs.insert(udi, disc);
    emit audioDiscDetected(udi);
  }

}

void CDDASolid::p_solid_device_removed(const QString& udi) {

  OpticalAudioDisc *disc = p_discs.value(udi, NULL);

  kDebug() << "Optical audio disc removed:" << udi;

  if (disc) delete disc;

  p_discs.remove(udi);
  emit audioDiscRemoved(udi);

}

bool CDDASolid::p_is_optical_audio_disc_in_drive(const Solid::Device& device) const {

  if (device.is<Solid::OpticalDisc>()) {
    const Solid::OpticalDisc *disc = device.as<Solid::OpticalDisc>();
    kDebug() << "Detected disc type:" << disc->discType();
    //TODO: There are some drives always reporting discType unknown (-1).
    //Needs some more testing
    /*if ((disc->discType() == Solid::OpticalDisc::CdRom) ||
      (disc->discType() == Solid::OpticalDisc::CdRecordable) ||
      (disc->discType() == Solid::OpticalDisc::CdRewritable)) {
      return (disc->availableContent() & Solid::OpticalDisc::Audio);
    }*/
    return (disc->availableContent() & Solid::OpticalDisc::Audio);
  }
  return FALSE;

}

void CDDASolid::p_clear() {

  QHash<QString, OpticalAudioDisc*>::const_iterator i = p_discs.constBegin();
  while (i != p_discs.constEnd()) {
    if (i.value()) delete i.value();
    ++i;
  }
  p_discs.clear();

}
