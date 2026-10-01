/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QStringList>
#include <QUrl>

#include <optional>

// The image listing of the Cover Art Archive for a release or release group
// (https://coverartarchive.org/release/<mbid>, .../release-group/<mbid>),
// see https://musicbrainz.org/doc/Cover_Art_Archive/API

namespace Audex
{

struct CoverArtImage {
    QUrl image; // the original upload
    QMap<int, QUrl> thumbnails; // 250, 500, 1200 pixels
    QStringList types; // "Front", "Back", "Booklet", "Medium", ...
    QString comment;
    bool front = false;

    // the thumbnail of that size (0 = the original), the next larger one if
    // it is missing, the original as last resort
    QUrl url(int size) const;
};

struct CoverArtListing {
    QList<CoverArtImage> images;
    QUrl release; // the release at MusicBrainz the images belong to
};

// Nothing if the data is not a listing. Addresses at (coverart)archive.org and
// musicbrainz.org are switched to https (the archive answers with http ones).
std::optional<CoverArtListing> parseCoverArtListing(const QByteArray &json, QString *error = nullptr);

}

Q_DECLARE_METATYPE(Audex::CoverArtListing)
