/*
 * AUDEX CDDA EXTRACTOR
 *
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
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

#include "cddadevices.h"

CDDADevices::CDDADevices(QObject *parent)
{

  Q_UNUSED(parent);

  connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(const QString&)), this, SLOT(p_solid_device_added(const QString&)));
  connect(Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(const QString&)), this, SLOT(p_solid_device_removed(const QString&)));

}

CDDADevices::~CDDADevices()
{

  p_clear();

}

const QString CDDADevices::blockDevice(const QString& udi) const
{

  OpticalAudioDisc *disc = p_discs.value(udi, NULL);
  if (!disc) return QString();

  Solid::Device device(disc->device.parentUdi());
  if (device.is<Solid::Block>()) return device.as<Solid::Block>()->device();

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

void CDDADevices::eject(const QString& udi)
{

  OpticalAudioDisc *disc = p_discs.value(udi, NULL);
  if (!disc) return;

  Solid::Device device(disc->device.parentUdi());
  device.as<Solid::OpticalDrive>()->eject();

}

void CDDADevices::setName(const QString& udi, const QString& name)
{

  OpticalAudioDisc *disc = p_discs.value(udi, NULL);
  if (!disc) return;

  if (disc->name != name) disc->name = name;

}

void CDDADevices::p_solid_device_added(const QString& udi)
{

  Solid::Device device(udi);

  kDebug() << "Device found:" << device.udi();
  if (p_is_optical_audio_disc(device))
  {
    kDebug() << "is audio.";
    OpticalAudioDisc *disc = new OpticalAudioDisc();
    disc->name = i18n("Audio Disc");
    disc->device = device;
    p_discs.insert(udi, disc);
    emit audioDiscDetected(udi);
  }

}

void CDDADevices::p_solid_device_removed(const QString& udi)
{

  OpticalAudioDisc *disc = p_discs.value(udi, NULL);

  if (disc)
  {
    kDebug() << "Optical audio disc removed:" << udi;
    delete disc;
    p_discs.remove(udi);
    emit audioDiscRemoved(udi);
  }

}

bool CDDADevices::p_is_optical_audio_disc(const Solid::Device& device) const
{

  if (device.is<Solid::OpticalDisc>())
    return (device.as<Solid::OpticalDisc>()->availableContent() & Solid::OpticalDisc::Audio);

  return false;

}

void CDDADevices::p_clear()
{

  QHash<QString, OpticalAudioDisc*>::const_iterator i = p_discs.constBegin();
  while (i != p_discs.constEnd())
  {
    if (i.value()) delete i.value();
    ++i;
  }
  p_discs.clear();

}
