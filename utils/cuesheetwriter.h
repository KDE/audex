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

#ifndef CUEFILEWRITER_H
#define CUEFILEWRITER_H

#include <QString>
#include <QStringList>
#include <QFileInfo>

#include <KDebug>
#include <KLocale>

#include "config.h"
#include "../models/cddamodel.h"

#define FRAMES2SEC(frames) ((frames) / 75)

class CueSheetWriter : public QObject {
  Q_OBJECT
public:
  CueSheetWriter(CDDAModel *model);
  ~CueSheetWriter();

  QStringList cueSheet(const QString& binFilename) const;
  QStringList cueSheet(const QStringList& filenames) const;

private:
  CDDAModel *model;

  QString p_filetype(const QString& filename) const;
  
};

#endif
