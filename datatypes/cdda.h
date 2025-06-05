/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "QString"

#include "metadata.h"
#include "toc.h"

namespace Audex
{

class CDDA
{
public:
    CDDA(const QString &udi = QString(), const Toc::Toc &toc = Toc::Toc(), const Metadata::Metadata &metadata = Metadata::Metadata());
    CDDA(const CDDA &other);
    CDDA &operator=(const CDDA &other);

    bool operator==(const CDDA &other) const;
    bool operator!=(const CDDA &other) const;

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
    int HTOALength() const; // in seconds
    const Metadata::Track &metadataHTOA() const
    {
        return p_metadata.track(0);
    }
    Metadata::Track &metadataHTOA()
    {
        return p_metadata.track(0);
    }

protected:
    QString p_udi;
    Toc::Toc p_toc;
    Metadata::Metadata p_metadata;
    Metadata::Track p_htoa_metadata;
};

}
