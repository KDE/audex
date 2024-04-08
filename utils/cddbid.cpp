/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddbid.h"

namespace Audex
{

quint32 CDDBId(const Toc::Toc &toc)
{
    return CDDBId(toc.trackOffsetList());
}

quint32 CDDBId(const TrackOffsetList &trackOffsetList)
{
    quint32 id = 0;
    for (int i = 0; i < trackOffsetList.count() - 1; ++i)
        id += digitsum(trackOffsetList[i] / 75);
    const quint32 length = FRAMES2SEC(trackOffsetList.last()) - FRAMES2SEC(trackOffsetList.first());
    return (((id % 0xff) << 24) | (length << 8) | (trackOffsetList.count() - 1));
}

}
