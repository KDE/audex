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

#ifndef CDDAEXTRACTTHREAD_HEADER
#define CDDAEXTRACTTHREAD_HEADER

#include <QString>
#include <QThread>

#include <KDebug>
#include <KLocale>

#include "cddaparanoia.h"

class CDDAExtractThread : public QThread {
  Q_OBJECT
public:
  CDDAExtractThread(QObject* parent = 0, const QString& device = "/dev/cdrom");
  ~CDDAExtractThread();

public slots:
  void start();
  void cancel();
  void setParanoiaMode(int mode) { paranoia_mode = mode; }
  void setMaxRetries(int r) { paranoia_retries = r; }
  void setNeverSkip(bool b) { never_skip = b; }
  void setTrackToRip(unsigned int t) { track = t; } // if t==0 rip whole cd
  void skipTrack(unsigned int t) { overall_sectors_read += paranoia->numOfFramesOfTrack(t); }

  bool isProcessing();

  const QStringList& protocol();

  CDDAParanoia *cddaParanoia() { return paranoia; }

private slots:
  void slot_error(const QString& message, const QString& details);

signals:
  void output(const QByteArray&);

  void progress(const int percent, const int sector, const int sectorsOverall);

  void error(const QString& message,
	const QString& details = QString());
  void warning(const QString& message);
  void info(const QString& message);

protected:
  /** reimplemented from QThread. Does the work */
  void run();

private:
  CDDAParanoia *paranoia;

  long first_sector;
  long current_sector;
  long last_sector;
  unsigned long sectors_read;
  unsigned long overall_sectors_read;
  unsigned long sectors_all;

  int paranoia_mode;
  int paranoia_retries;
  bool never_skip;

  unsigned int track;

  bool b_interrupt;
  bool b_error;

  QStringList extract_protocol;

  void createStatus(long, int);

  //status variables
  long last_read_sector;
  long overlap;
  long read_sectors;

  bool read_error;
  bool scratch_detected;

  //this friend function will call createStatus(long, int)
  friend void paranoiaCallback(long, int);

};

#endif
