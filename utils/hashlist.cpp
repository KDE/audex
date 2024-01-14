/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hashlist.h"

#include <QDebug>
#include <QTime>

Hashlist::Hashlist()
{
}

const QStringList Hashlist::getSFV(const QStringList &filenames)
{
    QStringList list;

    CRC32Hash checksum;
    for (int i = 0; i < filenames.count(); ++i) {
        QFile file(filenames.at(i));
        if (!file.exists())
            continue;
        if (!file.open(QFile::ReadOnly))
            continue;

        while (!file.atEnd())
            checksum.addData(file.read(HASHCALC_BUFSIZE));

        QFileInfo info(filenames.at(i));
        list << info.fileName() + ' ' + QString("%1").arg(checksum.result(), 8, 16, QLatin1Char('g')).toUpper();

        checksum.clear();

        file.close();
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

        while (!file.atEnd())
            md5sum.addData(file.read(HASHCALC_BUFSIZE));

        QFileInfo info(filenames.at(i));
        list << QString("%1").arg(QString(md5sum.result().toHex().toUpper())) + "  " + info.fileName();

        file.close();
    }

    return list;
}

const QStringList Hashlist::getSHA256(const QStringList &filenames)
{
    QStringList list;

    for (int i = 0; i < filenames.count(); ++i) {
        QFile file(filenames.at(i));
        if (!file.exists())
            continue;
        if (!file.open(QFile::ReadOnly))
            continue;

        QCryptographicHash sha256sum(QCryptographicHash::Sha256);

        while (!file.atEnd())
            sha256sum.addData(file.read(HASHCALC_BUFSIZE));

        QFileInfo info(filenames.at(i));
        list << QString("%1").arg(QString(sha256sum.result().toHex().toUpper())) + "  " + info.fileName();

        file.close();
    }

    return list;
}
