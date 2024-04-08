/* AUDEX CDInfo EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdinfo.h"

namespace Audex
{

CDInfo::CDInfo(const QString &udi, const Toc::Toc &toc, const Metadata::Metadata &metadata)
{
    p_udi = udi;
    p_toc = toc;
    p_metadata = metadata;
}

CDInfo::CDInfo(const CDInfo &other)
{
    p_toc = other.p_toc;
    p_metadata = other.p_metadata;
    p_udi = other.p_udi;
}

CDInfo &CDInfo::operator=(const CDInfo &other)
{
    p_toc = other.p_toc;
    p_metadata = other.p_metadata;
    p_udi = other.p_udi;

    return *this;
}

bool CDInfo::operator==(const CDInfo &other) const
{
    return p_toc == other.p_toc;
}

bool CDInfo::operator!=(const CDInfo &other) const
{
    return !(*this == other);
}

void CDInfo::clear()
{
    p_udi.clear();
    p_metadata.clear();
    p_toc.clear();
}

bool CDInfo::isEmpty() const
{
    return p_udi.isEmpty();
}

bool CDInfo::HTOAAvailable() const
{
    return p_toc.nullTrackAvailable();
}

int CDInfo::HTOASectorSize() const
{
    return p_toc.sectorCountOfNullTrack();
}

int CDInfo::HTOALength() const
{
    return p_toc.lengthOfNullTrack();
}

}
