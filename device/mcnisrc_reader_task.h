/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MCNISRC_READER_TASK_H
#define MCNISRC_READER_TASK_H

#include <QDebug>
#include <QString>
#include <QThread>
#include <QMap>

#include <KLocalizedString>

extern "C" {
#include <unistd.h>
}

#include "task.h"
#include "scsi.h"
#include "drive.h"

namespace Audex
{

namespace Device
{

typedef QMap<int, QByteArray> ISRCList;

class MCNISRCReaderTask : public Task
{
    Q_OBJECT

public:
    MCNISRCReaderTask(const QString &driveUDI);

    void enableReadMCN(const bool readMCN = true) {
        read_mcn = readMCN;
    }

    void setTracknumbersForReadISRC(const TracknumberSet &tracks = TracknumberSet());
    TracknumberSet tracknumbersForReadISRC() const;

Q_SIGNALS:
    void finished(const QString& driveUDI, const bool successful, const Metadata::Metadata metadata = Metadata::Metadata());

protected:
    void run() override;

private:
    bool read_mcn;
    TracknumberSet tracknumbers;

    bool is_all_zero(const QByteArray &data);
};

}

}

#endif
