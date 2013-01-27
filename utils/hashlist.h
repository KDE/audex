/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2013 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HASHLIST_H
#define HASHLIST_H

#include <sys/fcntl.h>
#include <sys/mman.h>

#include <unistd.h>

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QByteArray>
#include <QCryptographicHash>

#include <KDebug>

#include "3rdparty/crc32.h"

class Hashlist {

public:
  Hashlist();

  const QStringList getSFV(const QStringList& filenames);
  const QStringList getMD5(const QStringList& filenames);

};

#endif
