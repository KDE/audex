/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "task.h"

namespace Audex
{

namespace Device
{

Task::Task(const QString &driveUDI)
    : QThread()
{
    drive_udi = driveUDI;

    Solid::Device device(driveUDI);
    block_device = device.as<Solid::Block>()->device().toLatin1();
}

Message Task::lastError() const
{
    return last_error;
}

void Task::resetError()
{
    last_error.clear();
}

QStringList Task::logbook() const {
    return p_logbook;
}

void Task::resetLogbook() {
    p_logbook.clear();
}

void Task::log_entry(const Message &msg) {
    p_logbook.append(msg.text());
    if (msg.type() == Message::ERROR || msg.type() == Message::CRITICAL)
        last_error = msg;
    Q_EMIT log(drive_udi, msg);
}

}

}
