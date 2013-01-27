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

#include "cuesheetwriter.h"

CueSheetWriter::CueSheetWriter(CDDAModel *model) {

  this->model = model;

}

CueSheetWriter::~CueSheetWriter() {

}

QStringList CueSheetWriter::cueSheet(const QString& binFilename) const {

  QStringList result;
  
  result << "REM cue file written by Audex Version "AUDEX_VERSION;
  result << QString("REM GENRE \"%1\"").arg(model->genre());
  result << QString("REM DATE \"%1\"").arg(model->year());
  result << QString("PERFORMER \"%1\"").arg(model->artist());
  result << QString("TITLE \"%1\"").arg(model->title());
  
  QFileInfo info(binFilename);
  result << QString("FILE \"%1\" %2").arg(info.fileName()).arg(p_filetype(binFilename));
  
  const QList<unsigned> dsig = model->discSignature();
  
  int leadin = 0;
  for (int i = 0; i < dsig.count()-1; ++i) {
    if (!model->isAudioTrack(i+1)) continue;
    result << QString("  TRACK %1 AUDIO").arg(i+1, 2, 10, QChar('0'));
    result << QString("    PERFORMER \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString());
    result << QString("    TITLE \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString());
    if ((i == 0) && (dsig.at(i) > 0)) leadin = dsig.at(i);
    float l = (float)(dsig.at(i)-leadin) / 75.0f;
    int min = (int)l / 60;
    int sec = (int)l % 60;
    int frames = (dsig.at(i)-leadin) - (((min*60)+sec)*75);
    result << QString("    INDEX 01 %1:%2:%3").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(frames, 2, 10, QChar('0'));
  }
  
  return result;
  
}

QStringList CueSheetWriter::cueSheet(const QStringList& filenames) const {
  
  QStringList result;
  result << "REM cue file written by Audex Version "AUDEX_VERSION;
  result << QString("REM GENRE \"%1\"").arg(model->genre());
  result << QString("REM DATE \"%1\"").arg(model->year());
  result << QString("PERFORMER \"%1\"").arg(model->artist());
  result << QString("TITLE \"%1\"").arg(model->title());
  
  for (int i = 0; i < filenames.count(); ++i) {
    
     QFileInfo info(filenames.at(i));
     result << QString("FILE \"%1\" %2").arg(info.fileName()).arg(p_filetype(filenames.at(i)));
     
     result << QString("  TRACK %1 AUDIO").arg(i+1, 2, 10, QChar('0'));
     result << QString("    PERFORMER \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString());
     result << QString("    TITLE \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString());
     result << QString("    INDEX 01 00:00:00");
    
  }
  
  return result;
  
}


QString CueSheetWriter::p_filetype(const QString& filename) const {
  
  QString result = "WAVE";
  if ((filename.toLower().endsWith("aiff")) || (filename.toLower().endsWith("aif"))) result = "AIFF";
  else if (filename.toLower().endsWith("mp3")) result = "MP3";
  
  return result;
  
}
