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

#ifndef CACHEDIMAGE_H
#define CACHEDIMAGE_H

#include <QAbstractTableModel>
#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QString>
#include <QFile>

#include <KDebug>
#include <KMimeType>
#include <KLocale>

#include "utils/error.h"

class CachedImage {

public:
  CachedImage();
  CachedImage(const QByteArray& data);
  CachedImage(const QString& filename);
  CachedImage(const CachedImage& other);
  CachedImage& operator=(const CachedImage& other);
  bool operator==(const CachedImage& other) const;
  bool operator!=(const CachedImage& other) const;
  ~CachedImage();

  Error lastError() const;

  bool isEmpty() const;

  const QByteArray formatRaw() const;
  int dataSize(const QByteArray& format = "", const QSize& size = QSize());
  const QByteArray coverRaw() const;
  const QSize size() const;
  const QImage coverImage() const;
  quint16 checksum() const;

  const QString supportedMimeTypeList();
  const QList<QByteArray> supportedFormats();

  bool load(const QByteArray& data);
  bool load(const QString& filename);

  void clear();

  bool save(const QString& filename, const QSize& size = QSize());

private:
  QByteArray _format;
  QByteArray _data;
  QSize _size;

  bool compare_format(const QByteArray& f1, const QByteArray& f2) const;
  
  bool _save(QIODevice *device, const QByteArray& format = "", const QSize& size = QSize());
  
  QString mime_cache;

  Error _error;

};

#endif
