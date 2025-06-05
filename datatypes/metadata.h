/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef METADATA_H
#define METADATA_H

#include <QByteArray>
#include <QDebug>
#include <QImage>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "dataset.h"

namespace Audex
{

namespace Metadata
{

// https://www.xiph.org/vorbis/doc/v-comment.html
enum Type {
    Artist = 0,
    Composer,
    Performer,
    Album,
    Title,
    Work,
    Genre,
    Year,
    Comment,
    CDDBCategory, // string: blues, classical, country, data, folk, jazz, misc, newage, reggae, rock, soundtrack
    CDTextGenre,
    MCN,
    ISRC,
    Cover,
    VariousArtists, // boolean: CD with different artists per track, typically a sampler
    MultiDisc, // boolean: part of a disc collection
    DiscNumber, // int: disc number if part of a disc collection
    DiscCount, // int: number of discs if part of a disc collection
    TrackNumberOffset, // int: if part of a disc collection the track numbers might have an offset. E.g. if the first disc of a collection has 15 tracks, the
                       // tracknumber offset of the second disc should be 16
    TrackNumber,
    HiddenTrack,
    Invalid
};

typedef Dataset Track;

typedef QList<Track> TrackList;

class Metadata : public Dataset
{
public:
    const Track &track(const int tracknumber) const;
    Track &track(const int tracknumber);
    const Track &firstTrack() const;
    Track &firstTrack();
    const Track &lastTrack() const;
    Track &lastTrack();
    int appendTrack(const Track &track);
    int trackCount() const;

    void setCDDBCategory(const QString &category);

    const QImage cover() const;
    void setCover(const QImage &cover);
    bool loadCoverFromFile(const QString &filename);
    bool saveCoverToFile(const QString &filename);

    bool guessVarious() const;
    void setVarious(bool various);
    bool isVarious();

    int guessMultiDisc(QString &newAlbum) const;
    void setMultiDisc(const bool multi);
    bool isMultiDisc() const;
    void setDiscNum(const int n);
    int discNum() const;

    void swapArtistAndTitleOfTracks();
    void swapArtistAndAlbum();
    void splitTitleOfTracks(const QString &divider);
    void capitalizeTracks();
    void capitalizeHeader();
    void setTitleArtistsFromHeader();

    static TypeSet cddbRelevantTypes()
    {
        TypeSet result;
        result.insert(Artist);
        result.insert(Album);
        result.insert(Title);
        result.insert(Genre);
        result.insert(Year);
        result.insert(Comment);
        result.insert(CDDBCategory);
        return result;
    }

private:
    const QString capitalize(const QString &s);
    Track htoa;
};

QDebug operator<<(QDebug debug, const Metadata &metadata);

}

}

#endif
