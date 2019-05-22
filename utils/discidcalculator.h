/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <http://userbase.kde.org/Audex>
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

#ifndef DISCIDCALCULATOR_H
#define DISCIDCALCULATOR_H

#include <QObject>

#define FRAMES2SEC(frames) ((frames) / 75)

class DiscIDCalculator : public QObject {
  Q_OBJECT

public:
  static quint32 FreeDBId(const QList<quint32>& discSignature);
  static quint32 AccurateRipId1(const QList<quint32>& discSignature);
  static quint32 AccurateRipId2(const QList<quint32>& discSignature);

  static int p_checksum(int n) {
    /* a number like 2344 becomes 2+3+4+4 (13) */
    int ret = 0;
    while (n > 0) {
      ret = ret + (n % 10);
      n = n / 10;
    }
    return ret;
  }

};

#endif
