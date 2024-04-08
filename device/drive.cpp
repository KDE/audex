/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drive.h"

namespace Audex
{

namespace Device
{

Drive::Drive(const QString& udi)
{
    this->udi = udi;
    Solid::Device device(udi);
    block_device = device.as<Solid::Block>()->device().toLatin1();
}

}

}
