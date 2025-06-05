/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "upload.h"

#include <QDebug>
#include <QUrl>

namespace Audex
{

Upload::Upload(const QUrl &url, QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(parent);

    base_url = url;
}

void Upload::upload(const QString &targetpath, const QStringList &filelist)
{
    // first create the targetpath on the ftp server
    QString dir, olddir;
    int i = 0;
    Q_FOREVER {
        olddir = dir;
        dir = targetpath.section('/', 0, i, QString::SectionSkipEmpty);
        if (olddir == dir)
            break;
        ++i;
        QUrl url = base_url;
        url.setPath(base_url.path() + '/' + dir);
        // qDebug() << url;
        KIO::Job *job = KIO::mkdir(url);
        if (!job->exec()) {
            // if it already exists jump over
            if (job->error() != 113) {
                Q_EMIT error(job->errorText(), "");
                qDebug() << job->errorText();
                return;
            }
        }
    }

    for (int i = 0; i < filelist.count(); ++i) {
        QUrl url = base_url;
        QFileInfo fi(filelist.at(i));
        url.setPath(base_url.path() + '/' + targetpath + '/' + fi.fileName());
        Q_EMIT info(i18n("Uploading file %1 to server. Please wait...", fi.fileName()));
        KIO::Job *job = KIO::copy(QUrl(filelist.at(i)), url);
        if (!job->exec()) {
            Q_EMIT error(job->errorText(), "");
            qDebug() << job->errorText();
            return;
        }
        Q_EMIT info(i18n("Finished uploading file %1 to server.", fi.fileName()));
    }
}

}
