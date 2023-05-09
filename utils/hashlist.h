/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HASHLIST_H
#define HASHLIST_H

#include <sys/fcntl.h>
#include <sys/mman.h>

#include <unistd.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include "3rdparty/crc32.h"

class Hashlist
{
public:
    Hashlist();

    const QStringList getSFV(const QStringList &filenames);
    const QStringList getMD5(const QStringList &filenames);
};

#endif
