/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDebug>
#include <QSet>
#include <QString>

#include <KLocalizedString>

#include "dataset.h"

#define SECTORS_PER_SECOND 75
#define CD_SECTORSIZE_SAMPLES 588
#define LEADIN_SECTOR_COUNT 150

namespace Audex
{

typedef QList<quint32> DiscSignature; // sector offsets
typedef QSet<int> TracknumberSet;

typedef QList<QVariant> TrackIndicesList;

namespace Toc
{

enum Type {
    FirstSector = 0,
    LastSector,
    SessionNumber,
    TrackNumber,
    FirstTrackNumber,
    LastTrackNumber,
    Format,
    DataMode,
    PreEmphasis,
    CopyPermitted,
    MCN,
    ISRC,
    ChecksumCRC32,
    TrackIndices,
    Invalid
};

enum TrackFormat {
    AUDIO,
    DATA,
    UNKNOWN_FORMAT
};
enum TrackDataMode {
    MODE1,
    MODE2,
    UNKNOWN_DATAMODE
};

typedef Dataset Track;
typedef QList<Track> TrackList;

class Toc : public Dataset
{
public:
    const Track &track(const int tracknumber) const;
    Track &track(const int tracknumber);
    const Track &firstTrack() const;
    Track &firstTrack();
    const Track &lastTrack() const;
    Track &lastTrack();
    int appendTrack(const Track &track); // return tracknumber of appended track
    int trackCount() const;

    int firstTracknumber() const; // in almost all cases this is 1
    int lastTracknumber() const;

    int firstSectorOfDisc(const bool leadin = false) const;
    int lastSectorOfDisc(const bool leadin = false) const; // without lead-out

    int firstSectorOfTrack(const int tracknumber, const bool leadin = false) const;
    int lastSectorOfTrack(const int tracknumber, const bool leadin = false) const;

    int lastSectorOfLastAudioTrack(const bool leadin = false) const;

    int audioTrackCount() const;

    int lengthOfAudioTracks() const; // length of all audio tracks in seconds
    int sectorCountAudioTracks() const;

    int lengthOfTrack(const int tracknumber) const; // in seconds
    int lengthOfTracknumbers(const TracknumberSet &setOfTracknumbers) const; // in seconds
    int sectorCountTrack(const int tracknumber) const;
    int sectorCountTracknumbers(const TracknumberSet &setOfTracknumbers, const bool onlyAudioTracks = true) const;
    qreal sizeTrack(const int tracknumber) const; // in MiB

    // Do we have sectors before frist tracks starting sector? This indicates HTOA.
    bool nullTrackAvailable() const;
    int firstSectorOfNullTrack(const bool leadin = false) const;
    int lastSectorOfNullTrack(const bool leadin = false) const;
    int sectorCountOfNullTrack() const;
    int lengthOfNullTrack() const; // in seconds

    bool isAudioTrack(const int tracknumber) const;
    bool isDataTrack(const int tracknumber) const;
    bool isFirstTrack(const int tracknumber) const; // but not lead-in
    bool isLastTrack(const int tracknumber) const; // but not lead-out

    bool isAudioDisc() const; // at least one audio track?
    bool isPureAudioDisc() const; // all tracks are audio tracks?

    // First element is offset sector of first track after lead-in, list of sector offsets per track, last element sector offset of lead-out
    // lead-in: +150 sectors
    DiscSignature discSignature(const bool leadin = true, const bool skipDataTracks = false) const;

    const QStringList prettyTOC() const;

    bool trackPreEmphasis(const int tracknumber) const;
    bool trackCopyPermitted(const int tracknumber) const;

    const QString msfStringOfTrack(const int tracknumber, const bool leadin = false) const;

    void setFirstSectorOfDisc(const int first_sector_of_disc);
    void setLastSectorOfDisc(const int last_sector_of_disc);
};

// Frames == Sectors
const QString Frames2MSFString(const int lsn, const QChar sectorSeparator = u'-');
int MSF2Frames(const int min, const int sec, const int frames);
const QString Frames2TimeString(const int lsn, const QChar separator = u':'); // min:sec:msec

QDebug operator<<(QDebug debug, const Toc &toc);

}

}

Q_DECLARE_METATYPE(Audex::Toc::TrackFormat)
Q_DECLARE_METATYPE(Audex::Toc::TrackDataMode)
