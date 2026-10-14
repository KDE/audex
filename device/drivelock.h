/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>

namespace Audex
{

// Keeps other Audex processes (the GUI, audex-cli) off a drive while one of
// them uses it: an advisory flock() on the device node. Within a process the
// lock is shared, so threads of the same program do not lock each other out.
// Other programs (cdparanoia, a burner) do not look at it.
class DriveLock
{
public:
    DriveLock() = default;
    ~DriveLock();
    DriveLock(DriveLock &&other) noexcept;
    DriveLock &operator=(DriveLock &&other) noexcept;
    DriveLock(const DriveLock &) = delete;
    DriveLock &operator=(const DriveLock &) = delete;

    // Does not wait: fails at once if another process holds the drive.
    static DriveLock acquire(const QString &device, QString *error = nullptr);

    bool isLocked() const
    {
        return !m_device.isEmpty();
    }
    void release();

private:
    QString m_device;
};

}
