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

#include "parameters.h"

Parameters::Parameters() {

}

Parameters::Parameters(const Parameters& other) {
  p_parameters = other.p_parameters;
}

Parameters& Parameters::operator=(const Parameters& other) {
  p_parameters = other.p_parameters;
  return *this;
}

Parameters::~Parameters() {

}

void Parameters::fromString(const QString& string, const QChar& sep) {

  if (string.isEmpty()) return;

  p_parameters.clear();

  QString option;
  bool is_in_quote = FALSE;
  for (int i = 0; i < string.length(); ++i) {
    if (string[i]=='\'') {
      is_in_quote = !is_in_quote;
    } else if ((string[i]==sep) && (!is_in_quote)) {
      if (option.isEmpty()) continue;
      int equalPos = option.indexOf('=');
      if (equalPos==-1) {
        p_parameters[option] = "";
      } else {
        QString ropt = option.left(equalPos);
        QString val = option.mid(equalPos+1);
        p_parameters[ropt] = val.trimmed();
      }
      option.clear();
    } else {
      option += string[i];
    }
  }

  int equalPos = option.indexOf('=');
  if (equalPos==-1) {
    p_parameters[option] = "";
  } else {
    QString ropt = option.left(equalPos);
    QString val = option.mid(equalPos+1);
    p_parameters[ropt] = val.trimmed();
  }

}

const QString Parameters::toString(const QChar& sep) {

  QString string;

  QHash<QString,QString>::const_iterator i = p_parameters.constBegin();
  bool first = TRUE;
  while (i != p_parameters.constEnd()) {
    QString s = i.value();
    s.replace("'", "\\'");
    if (!first) {
      string += sep+i.key()+"='"+s+"'";
    } else {
      string += i.key()+"='"+s+"'";
      first = FALSE;
    }
    ++i;
  }

  return string;

}

void Parameters::fromBase64(const QByteArray& bytearray) {
  QByteArray ba = QByteArray::fromBase64(bytearray);
  fromString(QString::fromUtf8(ba.data()));
}

const QByteArray Parameters::toBase64() {
  QString s = toString();
  return s.toUtf8().toBase64();
}
