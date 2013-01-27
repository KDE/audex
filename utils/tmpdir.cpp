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

#include "tmpdir.h"

TmpDir::TmpDir(const QString& appName, const QString& sub) : QObject() {

  QStringList dirs = KGlobal::dirs()->resourceDirs("tmp");
  p_tmp_path_base = dirs.size()?dirs[0]:"/var/tmp/";
  kDebug() << "Found temporary path" << p_tmp_path_base;

  p_error = FALSE;

  PID pid;
  p_tmp_path = p_tmp_path_base;
  if (p_tmp_path.right(1) != "/") p_tmp_path += "/";
  p_tmp_path += appName+"."+QString("%1").arg(pid.getPID())+"/";
  p_tmp_path_app = p_tmp_path;
  if (!sub.isEmpty()) {
    p_tmp_path += sub+"/";
  }
  kDebug() << "Temporary folder in use:" << p_tmp_path;
  p_error = !p_create_dir(p_tmp_path);
  
}

TmpDir::~TmpDir() {

  //do we have a sub component in the path?
  if (p_tmp_path_app != p_tmp_path) {
    
    if (p_remove_dir(p_tmp_path)) {
      kDebug() << QString("Deleting temporary folder \"%1\"").arg(p_tmp_path);
    } else {
      kDebug() << QString("Deleting temporary folder \"%1\" failed").arg(p_tmp_path);
    }
  
  }
  
  QDir dir(p_tmp_path_app);
  if ((dir.exists()) && (!dir.rmdir(p_tmp_path_app))) {
    kDebug() << QString("Temporary folder \"%1\" not removed yet.").arg(p_tmp_path_app);
  }
  
}

const QString TmpDir::tmpPath() {
 
  p_error = !p_create_dir(p_tmp_path);
  return p_tmp_path;
  
}

quint64 TmpDir::freeSpace() const {
  
  KDiskFreeSpaceInfo diskfreespace = KDiskFreeSpaceInfo::freeSpaceInfo(p_tmp_path);
  return diskfreespace.available();
  
}

bool TmpDir::p_create_dir(const QString &dirName) {
 
  QDir *dir = new QDir(dirName);
  if (!dir->exists()) {
    if (!dir->mkpath(dirName)) {
      return FALSE;
    }
  }
  
  return TRUE;
  
}

bool TmpDir::p_remove_dir(const QString &dirName) {

  bool result = TRUE;
  QDir dir(dirName);
 
  if (dir.exists(dirName)) {

    Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
      if (info.isDir()) {
        result = p_remove_dir(info.absoluteFilePath());
      } else {
        result = QFile::remove(info.absoluteFilePath());
      }
      if (!result) {
        return result;
      }
    }
        
    result = dir.rmdir(dirName);
  
  }
 
  return result;

}
