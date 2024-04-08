/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CDINFO_H
#define CDINFO_H

#include "QString"

#include "metadata.h"
#include "toc.h"

namespace Audex
{

class CDInfo
{
public:
    CDInfo(const QString &udi = QString(), const Toc::Toc &toc = Toc::Toc(), const Metadata::Metadata &metadata = Metadata::Metadata());
    CDInfo(const CDInfo &other);
    CDInfo &operator=(const CDInfo &other);

    bool operator==(const CDInfo &other) const;
    bool operator!=(const CDInfo &other) const;

    const QString &udi() const
    {
        return p_udi;
    }
    QString &udi()
    {
        return p_udi;
    }

    void setToc(const Toc::Toc &toc)
    {
        p_toc = toc;
    }
    const Toc::Toc &toc() const
    {
        return p_toc;
    }
    Toc::Toc &toc()
    {
        return p_toc;
    }

    void setMetadata(const Metadata::Metadata &metadata)
    {
        p_metadata = metadata;
    }
    void mergeMetadata(const Metadata::Metadata &metadata)
    {
        p_metadata.merge(metadata);
    }
    const Metadata::Metadata &metadata() const
    {
        return p_metadata;
    }
    Metadata::Metadata &metadata()
    {
        return p_metadata;
    }

    void clear();
    bool isEmpty() const;

    bool HTOAAvailable() const;
    int HTOASectorSize() const;
    int HTOALength() const;
    const Metadata::Track &metadataHTOA() const
    {
        return p_htoa_metadata;
    }
    Metadata::Track &metadataHiddenTrack()
    {
        return p_htoa_metadata;
    }

protected:
    QString p_udi;
    Toc::Toc p_toc;
    Metadata::Metadata p_metadata;
    Metadata::Track p_htoa_metadata;
};

}

#endif
