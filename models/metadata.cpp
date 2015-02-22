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

#include "metadata.h"

Metadata::Metadata()
{

  p_readonly = false;

}

Metadata::Metadata(const Metadata& other)
{

  p_data = other.p_data;
  p_custom_data = other.p_custom_data;
  p_data_modified = other.p_data_modified;
  p_readonly = other.p_readonly;

}

Metadata& Metadata::operator=(const Metadata& other)
{

  p_data = other.p_data;
  p_custom_data = other.p_custom_data;
  p_data_modified = other.p_data_modified;
  p_readonly = other.p_readonly;
  return *this;

}

bool Metadata::operator==(const Metadata& other) const
{

  if (p_data != other.p_data) return false;
  return (p_custom_data == other.p_custom_data);

}

bool Metadata::operator!=(const Metadata& other) const
{

  if (p_data != other.p_data) return true;
  return (p_custom_data != other.p_custom_data);

}

Metadata::~Metadata()
{

}

const QVariant Metadata::get(const uint key, const QVariant& def) const
{

  return p_data.value(key, def);

}

bool Metadata::set(const uint key, const QVariant& d)
{

  if ((d == get(key)) || (p_readonly)) return false;
  p_data.insert(key, d);
  p_modify(key);
  return true;

}

bool Metadata::remove(const uint key)
{

  return (p_data.remove(key) > 0);

}

const QVariant Metadata::getCustom(const QString& label, const QVariant& def) const
{

  return p_custom_data.value(label, def);

}

bool Metadata::setCustom(const QString& label, const QVariant& d)
{

  if ((d == getCustom(label)) || p_readonly) return false;
  p_custom_data.insert(label, d);
  return true;

}

bool Metadata::removeCustom(const QString& label)
{

  return (p_custom_data.remove(label) > 0);

}

void Metadata::merge(const Metadata& other)
{

  Metadata tag = other;

  MetadataHash::const_iterator i = tag.data()->constBegin();
  while (i != tag.data()->constEnd())
  {
    set(i.key(), i.value());
    ++i;
  }

  QHash<QString, QVariant>::const_iterator j = tag.customData()->constBegin();
  while (j != tag.customData()->constEnd())
  {
    setCustom(j.key(), j.value());
    ++j;
  }

}

bool Metadata::isModified() const
{

  QHash<uint, bool>::const_iterator i = p_data_modified.constBegin();
  while (i != p_data_modified.constEnd())
  {
    if (i.value()) return true;
    ++i;
  }
  return false;

}

bool Metadata::isModified(const uint key) const
{

  return p_data_modified.value(key, false);

}

bool Metadata::isModified(const KeySet& keys) const
{

  KeySet::ConstIterator i = keys.constBegin();
  while (i != keys.constEnd())
  {
    if (isModified(*i)) return true;
  }
  return false;

}

bool Metadata::isValid(const uint key) const
{

  QVariant value = p_data.value(key, QVariant());
  bool ok = value.isValid();
  if (!ok) return false;
  if (value.type() == QVariant::String)
    return !value.toString().isEmpty();
  else
    return true;

}

void Metadata::clear()
{

  if (p_readonly) return;
  p_data.clear();
  p_custom_data.clear();
  p_data_modified.clear();

}

void Metadata::confirm()
{

  p_data_modified.clear();

}

void Metadata::swap(const uint key1, const uint key2)
{

  if (p_readonly) return;
  if (key1 == key2) return;

  QVariant tmp = get(key1);
  set(key1, get(key2));
  set(key2, tmp);

}

void Metadata::split(const uint skey, const QString& divider, const uint dkey1, const uint dkey2)
{

  if (p_readonly) return;
  if (divider.isEmpty()) return;
  if (get(skey).type() != QVariant::String) return;

  int splitPos = get(skey).toString().indexOf(divider);
  if (splitPos >= 0)
  {
    // split
    QString part1 = get(skey).toString().mid(splitPos+divider.length());
    QString part2 = get(skey).toString().left(splitPos);
    set(dkey1, part1);
    set(dkey2, part2);
  }

}

void Metadata::capitalize(const uint key)
{

  if (p_readonly) return;
  if (get(key).type() != QVariant::String) return;

  set(key, p_capitalize(get(key).toString()));

}

void Metadata::p_modify(const uint key)
{

  p_data_modified.insert(key, true);

}

const QString Metadata::p_capitalize(const QString &s)
{

  QStringList stringlist = s.split(" ", QString::SkipEmptyParts);
  for (int i = 0; i < stringlist.count(); ++i)
  {
    QString string = stringlist[i].toLower();
    int j = 0;
    while (((string[j] == '(') || (string[j] == '[') || (string[j] == '{')) && (j < string.length())) j++;
    string[j] = string[j].toUpper();
    stringlist[i] = string;
  }

  return stringlist.join(" ");

}
