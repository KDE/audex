/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QUrl>

#include <optional>

#include "fields.h"

namespace Audex::Metadata
{

// Distinct type, so album and track data cannot be mixed up by accident.
class Track : public Fields
{
};

// Cover image as delivered (file or download). The original bytes are kept,
// decoding happens in the GUI layer (QImage needs QtGui).
struct CoverArt {
    QByteArray data;
    QString mimeType; // "image/jpeg", "image/png", ...
    QString source; // file path or URL, informational
    QString origin; // where it comes from, for the user (empty: see source)
    QUrl originPage; // page about it, e.g. the release's cover art at MusicBrainz

    bool isNull() const
    {
        return data.isEmpty();
    }
    QString suffix() const; // "jpg", "png", ...

    static std::optional<CoverArt> fromData(const QByteArray &data, const QString &source = QString());
    static std::optional<CoverArt> fromFile(const QString &path, QString *error = nullptr);
    bool saveToFile(const QString &path, QString *error = nullptr) const;

    bool operator==(const CoverArt &other) const
    {
        return data == other.data && mimeType == other.mimeType;
    }
};

struct DiscHint {
    int number = 0;
    int count = 0; // 0 if unknown
    QString album; // album title without the disc suffix
};

// Metadata of one disc. Tracks are keyed by TOC track number; track 0 is the
// hidden track one audio (HTOA). Data tracks may have entries as well.
class Album : public Fields
{
public:
    using Fields::isModified; // keep isModified(Field) etc. visible

    bool hasTrack(int number) const;
    const Track &track(int number) const; // empty track if absent
    Track &track(int number); // created on demand, number must be 0..99
    void setTrack(int number, const Track &track);
    bool removeTrack(int number);
    QList<int> trackNumbers() const; // ascending
    const QMap<int, Track> &tracks() const
    {
        return m_tracks;
    }

    const CoverArt &cover() const
    {
        return m_cover;
    }
    bool setCover(const CoverArt &cover);
    bool removeCover();

    bool isEmpty() const; // no album fields, no non-empty tracks, no cover
    bool isModified() const; // album fields, any track, track set or cover
    void confirm();
    void reset();

    // Album fields, tracks (by number) and cover. With KeepExisting only
    // missing values are filled.
    bool merge(const Album &other, MergePolicy policy = MergePolicy::Overwrite);

    bool operator==(const Album &other) const;
    bool operator!=(const Album &other) const
    {
        return !(*this == other);
    }

    // --- helpers for the editing dialog (all tracks >= 1, HTOA excluded) ---

    // At least two tracks with different, non-empty artists
    bool guessVariousArtists() const;
    // "Album (CD 2)", "Album - Disc 1 of 2", "Album [disk 3]"
    std::optional<DiscHint> guessMultiDisc() const;

    bool swapTrackArtistsAndTitles();
    bool swapArtistAndAlbum();
    bool splitTrackTitles(const QString &divider); // "Artist<divider>Title"
    bool capitalizeTracks();
    bool capitalizeAlbum();
    bool setTrackArtistsFromAlbum(bool onlyEmpty = false);

private:
    QList<int> regularTrackNumbers() const;

    QMap<int, Track> m_tracks;
    CoverArt m_cover;
    bool m_structureModified = false;
};

QDebug operator<<(QDebug debug, const Album &album);

}
