/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DISCIDCALCULATOR_H
#define DISCIDCALCULATOR_H

#include <QList>

#define FRAMES2SEC(frames) ((frames) / 75)

class DiscIDCalculator
{
public:
    static quint32 CDDBId(const QList<quint32> &discSignature);
    static quint32 AccurateRipId1(const QList<quint32> &discSignature);
    static quint32 AccurateRipId2(const QList<quint32> &discSignature);

    static int p_digitsum(int n)
    {
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
