/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdda.h"

namespace Audex
{

CDDA::CDDA(const QString &udi, const Toc::Toc &toc, const Metadata::Metadata &metadata)
{
    p_udi = udi;
    p_toc = toc;
    p_metadata = metadata;
}

CDDA::CDDA(const CDDA &other)
{
    p_toc = other.p_toc;
    p_metadata = other.p_metadata;
    p_udi = other.p_udi;
    p_htoa_metadata = other.p_htoa_metadata;
}

CDDA &CDDA::operator=(const CDDA &other)
{
    p_toc = other.p_toc;
    p_metadata = other.p_metadata;
    p_udi = other.p_udi;
    p_htoa_metadata = other.p_htoa_metadata;

    return *this;
}

bool CDDA::operator==(const CDDA &other) const
{
    return p_toc == other.p_toc;
}

bool CDDA::operator!=(const CDDA &other) const
{
    return !(*this == other);
}

void CDDA::clear()
{
    p_udi.clear();
    p_metadata.clear();
    p_toc.clear();
}

bool CDDA::isEmpty() const
{
    return p_udi.isEmpty();
}

bool CDDA::HTOAAvailable() const
{
    return p_toc.nullTrackAvailable();
}

int CDDA::HTOASectorSize() const
{
    return p_toc.sectorCountOfNullTrack();
}

int CDDA::HTOALength() const
{
    return p_toc.lengthOfNullTrack();
}

}
