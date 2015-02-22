/*
 * AUDEX CDDA EXTRACTOR
 *
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
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
 *
 */

#ifndef METADATA_H
#define METADATA_H

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QHash>
#include <QSet>

#include <KLocale>

typedef QHash<uint, QVariant> MetadataHash;
typedef QSet<uint> KeySet;

class Metadata
{

public:
  Metadata();
  Metadata(const Metadata& other);
  Metadata& operator=(const Metadata& other);

  bool operator==(const Metadata& other) const;
  bool operator!=(const Metadata& other) const;

  ~MetaData();

  const QVariant get(const uint key, const QVariant& def = QVariant()) const;
  bool set(const uint key, const QVariant& d);
  bool remove(const uint key);

  const QVariant getCustom(const QString& label, const QVariant& def = QVariant()) const;
  bool setCustom(const QString& label, const QVariant& d);
  bool removeCustom(const QString& label);

  void merge(const Metadata& other);

  bool isModified() const;
  bool isModified(const uint key) const;
  //check if a set of labels was modified
  bool isModified(const KeySet& keys) const;

  bool isValid(const uint key) const;

  //flush all data
  void clear();
  //data will not be indicated as modified any longer
  void confirm();

  //if read only there is no modification possible
  //can be bypassed by accessing data()
  inline void setReadOnly(bool ro = true) { p_readonly = ro; }
  inline bool isReadOnly() const { return p_readonly; }

  void swap(const uint key1, const uint key2);
  //splits skey by divider into dkey1 (part before divider) and dkey2 (part after divider)
  void split(const uint skey, const QString& divider, const uint dkey1, const uint dkey2);
  void capitalize(const uint key);

  inline MetadataHash* data() { return &p_data; }
  inline QHash<QString, QVariant>* customData() { return &p_custom_data; }

protected:
  MetadataHash p_data;
  QHash<uint, bool> p_data_modified;

  QHash<QString, QVariant> p_custom_data;

  void p_modify(const uint key);

  bool p_readonly;

  const QString p_capitalize(const QString &s);

};

#endif
