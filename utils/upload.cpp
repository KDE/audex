/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
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

 #include "upload.h"

#include <QDebug>
#include <QUrl>

Upload::Upload(const QUrl &url, QObject *parent) : QObject(parent) {

  Q_UNUSED(parent);

  base_url = url;

}

Upload::~Upload() {

}

void Upload::upload(const QString& targetpath, const QStringList& filelist) {

  //first create the targetpath on the ftp server
  QString dir, olddir;
  int i = 0;
  forever {
    olddir = dir;
    dir = targetpath.section('/', 0, i, QString::SectionSkipEmpty);
    if (olddir==dir) break;
    ++i;
    QUrl url = base_url;
    url.setPath(base_url.path()+'/'+dir);
    //qDebug() << url;
    KIO::Job *job = KIO::mkdir(url);
    if (!job->exec()) {
      //if it already exists jump over
      if (job->error()!=113) {
        emit error(job->errorText(), "");
	qDebug() << job->errorText();
        return;
      }
    }
  }

  for (int i = 0; i < filelist.count(); ++i) {
    QUrl url = base_url;
    QFileInfo fi(filelist.at(i));
    url.setPath(base_url.path()+'/'+targetpath+'/'+fi.fileName());
    emit info(i18n("Uploading file %1 to server. Please wait...", fi.fileName()));
    KIO::Job *job = KIO::copy(QUrl(filelist.at(i)), url);
    if (!job->exec()) {
      emit error(job->errorText(), "");
      qDebug() << job->errorText();
      return;
    }
    emit info(i18n("Finished uploading file %1 to server.", fi.fileName()));
  }

}
