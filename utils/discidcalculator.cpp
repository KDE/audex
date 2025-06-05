/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "discidcalculator.h"

namespace Audex
{

quint32 DiscIDCalculator::CDDBId(const QList<quint32> &discSignature)
{
    quint32 cksum = 0;
    for (int i = 0; i < discSignature.count() - 1; ++i)
        cksum += p_digitsum(discSignature[i] / 75);
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

}
