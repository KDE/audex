/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QList>
#include <QString>
#include <QUrl>

#include "metadata.h"

namespace Audex
{

// One possible set of metadata for a disc, as delivered by a provider.
// The user (or a policy) picks one; the lookup never decides silently.
struct MetadataCandidate {
    QString provider; // provider id: "cdtext", "musicbrainz", "kcddb", ...
    QString providerName; // for display
    QString reference; // provider specific: MusicBrainz release id, CDDB "category/discid"
    int score = 0; // 0..100: how certain it is that this entry describes this disc
    Metadata::Album album; // tracks keyed by TOC track number
    // A cover that can be fetched on request
    struct Cover {
        QUrl url;
        QUrl page; // where the user can look at it (MusicBrainz)
        bool releaseGroup = false; // chosen for the release group: the release has none of its own
    };
    QList<Cover> covers; // in order of preference

    // "Artist – Album (1998, GB, disc 1/2)"
    QString description() const;
};

using MetadataCandidates = QList<MetadataCandidate>;

// Highest score first, stable for equal scores
void sortCandidates(MetadataCandidates &candidates);

}
