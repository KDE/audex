/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TOC_READER_TASK_H
#define TOC_READER_TASK_H

#include <unistd.h>

#include <QDebug>
#include <QString>
#include <QThread>

#include <KLocalizedString>

extern "C" {
#include <unistd.h>
}

#include "task.h"
#include "scsi.h"

#include "datatypes/toc.h"

namespace Audex
{

namespace Device
{

const unsigned char MMC_READ_TOC_PMA_ATIP = 0x43;

struct track_descriptor_format_2h {
    unsigned char session_number;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
    unsigned char adr : 4;
    unsigned char control : 4;
#else
    unsigned char control : 4;
    unsigned char adr : 4;
#endif
    unsigned char tno;
    unsigned char point;
    unsigned char min;
    unsigned char sec;
    unsigned char frame;
    unsigned char zero;
    unsigned char p_min;
    unsigned char p_sec;
    unsigned char p_frame;
};

class TocReaderTask : public Task
{
    Q_OBJECT

public:
    TocReaderTask(const QString &driveUDI) : Task(driveUDI) {}

Q_SIGNALS:
    void finished(const QString &driveUDI, const bool successful, const Toc::Toc toc = Toc::Toc());

protected:
    void run() override;
};

}

}

#endif
