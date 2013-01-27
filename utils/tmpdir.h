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

#ifndef TMPDIR_H
#define TMPDIR_H

#include <QObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>

#include <KGlobal>
#include <KStandardDirs>
#include <KDebug>
#include <KDiskFreeSpaceInfo>

#include "utils/pid.h"

class TmpDir : public QObject {

public:
  TmpDir(const QString& appName, const QString& sub = "");
  ~TmpDir();

  const QString tmpPath();
  inline const QString tmpPathBase() const { return p_tmp_path_base; }
  inline bool error() const { return p_error; }
  
  quint64 freeSpace() const;

private:
  QString p_tmp_path_base; //e.g. /var/tmp
  QString p_tmp_path_app; //e.g. /var/tmp/audex.1234
  QString p_tmp_path; //e.g. /car/tmp/audex.1234/sub
  bool p_error;

  bool p_create_dir(const QString &dirName);
  bool p_remove_dir(const QString &dirName);

};

#endif
