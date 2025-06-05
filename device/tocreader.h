/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unistd.h>

#include <QDebug>
#include <QString>
#include <QThread>

#include <Solid/Block>
#include <Solid/Device>

#include <KLocalizedString>

#include "datatypes/toc.h"

extern "C" {
#include <unistd.h>
}

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

const Toc::Toc readTOC(const QString &discUDI, QString &error);

}

}
