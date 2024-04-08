/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CDTEXT_READER_TASK_H
#define CDTEXT_READER_TASK_H

#include <QDebug>
#include <QString>
#include <QThread>

extern "C" {
#include <unistd.h>
}

#include <KLocalizedString>

#include "task.h"
#include "scsi.h"

#include "datatypes/metadata.h"
#include "utils/crc.h"

namespace Audex
{

namespace Device
{

struct cdtext_pack {
    unsigned char id1;
    unsigned char id2;
    unsigned char id3;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
    unsigned char dbcc : 1;
    unsigned char blocknum : 3;
    unsigned char charpos : 4;
#else
    unsigned char charpos : 4;
    unsigned char blocknum : 3;
    unsigned char dbcc : 1;
#endif
    unsigned char data[12];
    unsigned char crc[2];
};

class CDTextReaderTask : public Task
{
    Q_OBJECT

public:
    CDTextReaderTask(const QString &driveUDI) : Task(driveUDI) {}

Q_SIGNALS:
    void finished(const QString& driveUDI, const bool successful, const Metadata::Metadata& metadata = Metadata::Metadata());

protected:
    void run() override;

private:
    void debug_raw_text_pack_data(const QByteArray &data);

    typedef QHash<int, QHash<int, QString>> Cache;
};

}

}

#endif
