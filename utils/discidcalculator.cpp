/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
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

#include "discidcalculator.h"

quint32 DiscIDCalculator::FreeDBId(const QList<quint32> &discSignature)
{
    quint32 cksum = 0;
    for (int i = 0; i < discSignature.count() - 1; ++i)
        cksum += p_checksum(discSignature[i] / 75);
    return (((cksum % 0xff) << 24) | (FRAMES2SEC(discSignature.last() - discSignature.first()) << 8) | (discSignature.count() - 1));
}

quint32 DiscIDCalculator::AccurateRipId1(const QList<quint32> &discSignature)
{
    quint32 result = 0;
    for (int i = 0; i < discSignature.count(); ++i) {
        result += discSignature[i] - discSignature.first();
    }
    return result;
}

quint32 DiscIDCalculator::AccurateRipId2(const QList<quint32> &discSignature)
{
    quint32 result = 1;
    for (int i = 1; i < discSignature.count(); ++i)
        result += ((discSignature[i] - discSignature.first()) * (i + 1));
    return result;
}
