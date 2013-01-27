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

#include "cachedimage.h"

CachedImage::CachedImage() {
}

CachedImage::CachedImage(const QByteArray& data) {
  load(data);
}

CachedImage::CachedImage(const QString& filename) {
  load(filename);
}

CachedImage::CachedImage(const CachedImage& other) {
  _data = other._data;
  _format = other._format;
  mime_cache = other.mime_cache;
}

CachedImage& CachedImage::operator=(const CachedImage& other) {
  _data = other._data;
  _format = other._format;
  mime_cache = other.mime_cache;
  return *this;
}

bool CachedImage::operator==(const CachedImage& other) const {
  return (this->checksum() == other.checksum());
}

bool CachedImage::operator!=(const CachedImage& other) const {
  return (this->checksum() != other.checksum());
}

CachedImage::~CachedImage() { };

Error CachedImage::lastError() const {
  return _error;
}

bool CachedImage::isEmpty() const {
  return (_data.size()==0);
}

const QByteArray CachedImage::formatRaw() const {
  return _format;
}

int CachedImage::dataSize(const QByteArray& format, const QSize& size) {
  if ((compare_format(format, _format) || (format.isEmpty())) && ((size.isNull()) || (size == _size))) {
    return _data.size();
  } else {
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    _save(&buffer, format, size);
    buffer.close();
    return ba.size();
  }
}

const QByteArray CachedImage::coverRaw() const {
  return _data;
}

const QSize CachedImage::size() const {
  return _size;
}

const QImage CachedImage::coverImage() const {
  return QImage::fromData(_data);
}

quint16 CachedImage::checksum() const {
  return qChecksum(_data.data(), _data.size());
}

const QString CachedImage::supportedMimeTypeList() {
  if (mime_cache.isEmpty()) {
    QList<QByteArray> supp_list = QImageReader::supportedImageFormats();
    QMap<QString,QStringList> map;
    for (int i = 0; i < supp_list.count(); ++i) {
      map[KMimeType::findByUrl("dummy."+QString(supp_list[i]).toLower())->comment()].append("*."+QString(supp_list[i]).toLower());
    }
    QString result = "*.jpg *.jpeg *.png *.gif|"+i18n("Common image formats")+" (*.jpg, *.jpeg, *.png, *.gif)";
    QMap<QString,QStringList>::const_iterator i = map.constBegin();
    while (i != map.constEnd()) {
       if (i.key()==KMimeType::defaultMimeTypePtr()->comment()) { ++i; continue; }
       result += "\n";
       QStringList extensions = i.value();
       extensions.removeDuplicates();
       result += extensions.join(" ")+"|"+i.key()+" ("+extensions.join(", ")+")";
      ++i;
    }
    mime_cache = result;
  }
  return mime_cache;
}

const QList<QByteArray> CachedImage::supportedFormats() {
  return QImageReader::supportedImageFormats();
}

bool CachedImage::load(const QByteArray& data) {
  QBuffer buffer((QByteArray*)&data);
  buffer.open(QIODevice::ReadOnly);
  QImageReader ir(&buffer);
  _format = ir.format();
  _size = ir.size();
  buffer.close();
  kDebug() << "Load cover image from buffer (" << _format << ")";
  if (!_format.isEmpty() && QImageReader::supportedImageFormats().contains(_format)) {
    _data = data;
    return TRUE;
  }
  _format = "";
  _size = QSize();
  return FALSE;
}

bool CachedImage::load(const QString& filename) {
  QFile file(filename);
  if (!file.exists()) {
    _error = Error(i18n("File does not exist."), i18n("Please check if the file really exists."), Error::ERROR);
    return FALSE;
  }
  QImageReader ir(filename);
  _format = ir.format();
  _size = ir.size();
  kDebug() << "Load cover image from file (" << _format << ")";
  if (!_format.isEmpty() && QImageReader::supportedImageFormats().contains(_format)) {
    if (!file.open(QIODevice::ReadOnly)) {
      _error = Error(i18n("Cannot open file."), i18n("Please check your file. Maybe you do not have proper permissions."), Error::ERROR);
      return FALSE;
    }
    _data = file.readAll();
    file.close();
  } else {
    _error = Error(i18n("Unsupported image format"), i18n("Please check your file. Maybe it is corrupted. Otherwise try to convert your cover image with an external program to a common file format like JPEG."), Error::ERROR);
    _format = "";
    _size = QSize();
    return FALSE;
  }
  _error = Error();
  return TRUE;
}

void CachedImage::clear() {
  _data.clear();
  _format.clear();
}

bool CachedImage::save(const QString& filename, const QSize& size) {
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
    _error = Error(i18n("Cannot open file"), i18n("Please check your permissions."), Error::ERROR);
    return FALSE;
  }
  QByteArray format = KMimeType::extractKnownExtension(filename).toLower().toAscii();
  if ((compare_format(format, _format) || (format.isEmpty())) && ((size.isNull()) || (size == _size))) {
    qint64 r = file.write(_data);
    if ((r==-1) || (r < _data.size())) {
      _error = Error(i18n("Cannot save file"), i18n("Please check your permissions. Do you have enough space?"), Error::ERROR);
      file.close();
      return FALSE;
    }
  } else {
    if (!_save(&file, format, size)) {
      file.close();
      return FALSE; 
    }
  }
  file.close();
  _error = Error();
  return TRUE;
}

bool CachedImage::compare_format(const QByteArray& f1, const QByteArray& f2) const {
  if (((f1.toLower() == "jpg") || (f1.toLower() == "jpeg")) && ((f2.toLower() == "jpg") || (f2.toLower() == "jpeg"))) return TRUE;
  if (((f1.toLower() == "tif") || (f1.toLower() == "tiff")) && ((f2.toLower() == "tif") || (f2.toLower() == "tiff"))) return TRUE;
  return (f1.toLower() == f2.toLower());
}

bool CachedImage::_save(QIODevice *device, const QByteArray& format, const QSize& size) {
  if ((!format.isEmpty()) && (!QImageReader::supportedImageFormats().contains(format))) {
    _error = Error(i18n("Unsupported image format"), i18n("Please use common image formats like JPEG, PNG or GIF as they are supported on almost all systems."), Error::ERROR);
    return FALSE;
  }
  QImage image = QImage::fromData(_data);
  if ((size.isValid()) && (image.size() != size)) image = image.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  bool ok;
  if (!format.isEmpty()) ok = image.save(device, format.data()); else ok = image.save(device);
  if (!ok) {
    _error = Error(i18n("Cannot write on device"), i18n("Please check your permissions. Do you have enough space on your device?"), Error::ERROR);
    return FALSE;
  }
  _error = Error();
  return TRUE;
}
