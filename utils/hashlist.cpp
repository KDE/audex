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
 
#include "hashlist.h"

Hashlist::Hashlist() {

}

const QStringList Hashlist::getSFV(const QStringList& filenames) {

  QStringList list;

  for (int i = 0; i < filenames.count(); ++i) {

    //uses mmap for performance
    int fd = open(filenames.at(i).toUtf8().constData(), O_RDONLY);
    if (fd == -1) continue;

    quint64 size = lseek(fd, 0, SEEK_END);

    char *t_data = (char*)mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
    
    CRC32 crc32;
    crc32.update((const unsigned char *)t_data, (int)lseek(fd, 0, SEEK_END));
    
    QFileInfo info(filenames.at(i));
    list << info.fileName()+" "+QString("%1").arg(crc32.result(), 0, 16);

    close(fd);

  }

  return list;

}

const QStringList Hashlist::getMD5(const QStringList& filenames) {

  QStringList list;

  for (int i = 0; i < filenames.count(); ++i) {

    QFile file(filenames.at(i));
    if (!file.exists()) continue;
    if (!file.open(QFile::ReadOnly)) continue;

    QCryptographicHash md5sum(QCryptographicHash::Md5); 

    QByteArray buf;
    
    while (!file.atEnd()) {
      buf = file.read(16*1024);
      md5sum.addData(buf);
    }

    QFileInfo info(filenames.at(i));
    list << QString("%1").arg(QString(md5sum.result().toHex()))+"  "+info.fileName();

    file.close();

  }

  return list;

}
