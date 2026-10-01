/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>

#include "core/drivefeatures.h"

// Per-drive read settings, stored in the application config in groups
// "Device <udi>" (udi = Solid UDI, stable across reboots and device node
// renames). Drives without stored values fall back per key to the global
// Preferences values, so the existing global settings become the defaults
// for every newly seen drive (migration).
class DeviceSettings
{
public:
    struct Values {
        bool secureMode;
        bool useC2;
        bool cacheDefeat;
        bool overread;
        int retriesOnReadError;
        int sampleShift;
        int readSpeed;
        int errorReadSpeed;

        bool operator==(const Values &) const = default;
    };

    static Values defaults(); // from the global (legacy) Preferences
    static Values factoryDefaults(); // from audex.kcfg, for the "Defaults" button
    static Values load(const QString &udi);
    static void save(const QString &udi, const Values &values);

    // Far reads per cache defeat, measured by the first rip (0 = not measured)
    static int cacheDefeatReads(const QString &udi);
    static void setCacheDefeatReads(const QString &udi, int reads);

    // What the drive assistant measured: properties of the drive, not settings
    // the user picks. The list of checks is not stored.
    static Audex::Rip::DriveFeatures features(const QString &udi);
    static void setFeatures(const QString &udi, const Audex::Rip::DriveFeatures &features);
};
