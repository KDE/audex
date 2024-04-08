/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DISCID_H
#define DISCID_H

#include <QObject>

#include "datatypes/toc.h"

#define FRAMES2SEC(frames) ((frames) / 75)

namespace Audex
{

quint32 CDDBId(const Toc::Toc &toc);
quint32 CDDBId(const TrackOffsetList &trackOffsetList);

inline int digitsum(int n)
{
    int ret = 0;
    while (n > 0) {
        ret = ret + (n % 10);
        n = n / 10;
    }
    return ret;
};

}

#endif
