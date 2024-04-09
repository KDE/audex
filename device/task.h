/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DEVICE_TASK_H
#define DEVICE_TASK_H

#include <QString>
#include <QThread>
#include <QAtomicInteger>

#include <Solid/Block>
#include <Solid/Device>

#include <KLocalizedString>

#include "datatypes/message.h"

#include "drive.h"

namespace Audex
{

namespace Device
{

class Task : public QThread
{
    Q_OBJECT
public:
    Task(const QString &driveUDI);

    Message lastError() const;
    void resetError();

    QStringList logbook() const;
    void resetLogbook();

Q_SIGNALS:
    void started(const QString &driveUDI);
    void finished(const QString &driveUDI, const bool successful);

    void log(const QString &driveUDI, const Message &msg);

protected:
    QString drive_udi;
    QByteArray block_device;

    Message last_error;

    void log_entry(const Message &msg);

    QStringList p_logbook;
};

}

}

#endif
