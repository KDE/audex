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

#ifndef UPLOAD_H
#define UPLOAD_H

#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>

#include <KDebug>
#include <KLocale>
#include <KUrl>
#include <KIO/Job>
#include <KIO/SimpleJob>
#include <KIO/CopyJob>

class Upload : public QObject {
  Q_OBJECT
public:
  Upload(const KUrl& url, QObject *parent = 0);
  ~Upload();

  void upload(const QString& targetpath, const QStringList& filelist);

signals:
  void error(const QString& message,
	const QString& solution = QString());
  void warning(const QString& message);
  void info(const QString& message);

private:
  KUrl base_url;

};

#endif
