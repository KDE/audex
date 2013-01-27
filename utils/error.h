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

#ifndef ERROR_H
#define ERROR_H

class Error {

public:

  enum ErrorType { ERROR, WARNING };

  Error(const QString& message = "", const QString& details = "", const ErrorType errorType = Error::ERROR, QObject *parent = 0) {
    Q_UNUSED(parent);
    m = message;
    d = details;
    e = errorType;
  }
  Error(const Error& other) {
    m = other.m;
    d = other.d;
    e = other.e;
  }
  Error& operator=(const Error& other) {
    m = other.m;
    d = other.d;
    e = other.e;
    return *this;
  }
  ~Error() {

  }

  ErrorType errorType() const {
    return e;
  }

  bool isValid() const {
    return (!m.isEmpty());
  }

  const QString message() const {
    return m;
  }
  const QString details() const {
    return d;
  }

private:
  ErrorType e;
  QString m;
  QString d;

};

typedef QList<Error> ErrorList;

#endif
