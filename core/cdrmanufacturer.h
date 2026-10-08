/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2009-2013 Thomas Schmitt <scdbackup@gmx.net>
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>

namespace Audex::Mmc
{

// Manufacturer of a CD-R/RW by the lead-in start of its ATIP (see Atip in
// mmcparse.h); empty if unknown. Table and lookup from libburn, see
// cdrmanufacturer.cpp.
QString cdrManufacturer(int minute, int second, int frame);

}
