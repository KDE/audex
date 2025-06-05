/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <sys/fcntl.h>
#include <sys/mman.h>

#include <unistd.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTime>

#define HASHCALC_BUFSIZE 4 * 1024 * 1024

namespace Audex
{

class Hashlist
{
public:
    Hashlist();

    const QStringList getSFV(const QStringList &filenames);
    const QStringList getMD5(const QStringList &filenames);
    const QStringList getSHA256(const QStringList &filenames);
};

}
