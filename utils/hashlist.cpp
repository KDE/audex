/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hashlist.h"

Hashlist::Hashlist()
{
}

const QStringList Hashlist::getSFV(const QStringList &filenames)
{
    QStringList list;

    for (int i = 0; i < filenames.count(); ++i) {
        // uses mmap for performance
        int fd = open(filenames.at(i).toUtf8().constData(), O_RDONLY);
        if (fd == -1)
            continue;

        quint64 size = lseek(fd, 0, SEEK_END);

        char *t_data = (char *)mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);

        CRC32 crc32;
        crc32.update((const unsigned char *)t_data, (int)lseek(fd, 0, SEEK_END));

        QFileInfo info(filenames.at(i));
        list << info.fileName() + ' ' + QString("%1").arg(crc32.result(), 0, 16);

        close(fd);
    }

    return list;
}

const QStringList Hashlist::getMD5(const QStringList &filenames)
{
    QStringList list;

    for (int i = 0; i < filenames.count(); ++i) {
        QFile file(filenames.at(i));
        if (!file.exists())
            continue;
        if (!file.open(QFile::ReadOnly))
            continue;

        QCryptographicHash md5sum(QCryptographicHash::Md5);

        QByteArray buf;

        while (!file.atEnd()) {
            buf = file.read(16 * 1024);
            md5sum.addData(buf);
        }

        QFileInfo info(filenames.at(i));
        list << QString("%1").arg(QString(md5sum.result().toHex())) + "  " + info.fileName();

        file.close();
    }

    return list;
}
