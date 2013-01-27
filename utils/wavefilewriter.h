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

#ifndef WAVEFILEWRITER_HEADER
#define WAVEFILEWRITER_HEADER

#include <QString>
#include <QFile>
#include <QDataStream>

#include <KDebug>
#include <KLocale>

class WaveFileWriter : public QObject {
  Q_OBJECT
public:
  WaveFileWriter();
  ~WaveFileWriter();

  enum Endianess { BigEndian, LittleEndian };

  /**
   * open a new wave file.
   * closes any opened file.
   */
  bool open(const QString& filename);
  bool isOpen();
  QString filename() const;

  void setEndianess(const Endianess e);
  Endianess endianess();

  /**
   * closes the file.
   * Length of the wave file will be written into the header.
   * If no data has been written to the file except the header
   * it will be removed.
   */
  void close();

public slots:
  /**
   * write 16bit samples to the file.
   */
  void write(const QByteArray& data);

signals:
  void error(const QString& errorstr);

private:
  void p_write_empty_header();
  void p_update_header();

  Endianess p_endianess;

  QFile p_output_file;
  QDataStream p_output_stream;
  QString p_filename;

  long p_data_written;

};

#endif
