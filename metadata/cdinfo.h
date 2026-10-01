/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "core/toc.h"
#include "metadata/candidate.h"
#include "metadata/metadata.h"

#include <QList>
#include <QString>

#include <optional>

namespace Audex
{

// One row of the disc: a TOC track or the hidden track one audio.
struct DiscEntry {
    int number = 0; // TOC track number, 0 = hidden track one audio
    bool audio = true;
    bool hidden = false;
    int session = 1;
    int firstLba = 0;
    int lastLba = -1; // inclusive

    int sectorCount() const
    {
        return lastLba - firstLba + 1;
    }
};

// Everything known about the disc in a drive: TOC, identifiers and the
// current metadata. A plain value type: copyable, comparable, usable across
// threads and in signals. Fetching metadata is done elsewhere
// (MetadataLookup); this class only knows how to apply a result.
class CDInfo
{
public:
    CDInfo() = default;
    explicit CDInfo(const Cdda::Toc &toc, const QString &discUdi = QString());

    bool isEmpty() const
    {
        return m_toc.isEmpty();
    }
    void clear();

    const Cdda::Toc &toc() const
    {
        return m_toc;
    }
    QString discUdi() const
    {
        return m_discUdi;
    }
    void setDiscUdi(const QString &udi)
    {
        m_discUdi = udi;
    }

    // --- identifiers (computed from the TOC) ---
    QString cddbDiscId() const
    {
        return m_cddbDiscId;
    }
    QString musicBrainzDiscId() const
    {
        return m_musicBrainzDiscId;
    }
    QString musicBrainzTocString() const
    {
        return m_musicBrainzToc;
    }

    // --- disc layout ---
    bool hasHtoa() const;
    int htoaSectorCount() const;
    QList<DiscEntry> entries() const; // HTOA first (if any), then all TOC tracks
    std::optional<DiscEntry> entry(int number) const;
    bool hasEntry(int number) const;
    bool isAudioTrack(int number) const; // 0 = HTOA
    QList<int> audioTrackNumbers(bool includeHtoa = false) const;
    // Audio tracks flagged with pre-emphasis in the TOC
    QList<int> preEmphasisTracks() const;

    // Databases count audio tracks 1..n, the TOC may not (Mixed Mode CD:
    // data track 1, audio from track 2). position is 1-based.
    int trackNumberForAudioPosition(int position) const; // 0 if invalid
    int audioPositionOfTrack(int number) const; // 0 if not an audio track

    // Track number as shown and tagged: TrackNumberOffset applied, HTOA stays 0
    int displayTrackNumber(int number) const;

    // --- the medium (from the drive: ATIP, current profile) ---
    enum class Medium {
        Unknown,
        Pressed,
        CdR,
        CdRw
    };
    Medium medium() const
    {
        return m_medium;
    }
    QString mediumDetails() const // e.g. "lead-in 97:24:01, Taiyo Yuden Company Limited"
    {
        return m_mediumDetails;
    }
    void setMedium(Medium medium, const QString &details = QString())
    {
        m_medium = medium;
        m_mediumDetails = details;
    }
    QString mediumDescription() const; // for logs: "CD-R (lead-in ...)", "pressed CD", "unknown"
    // CD+G graphics in the sub-channel (background detection); nothing = not checked
    std::optional<bool> cdg() const
    {
        return m_cdg;
    }
    void setCdg(std::optional<bool> found)
    {
        m_cdg = found;
    }

    // --- metadata ---
    const Metadata::Album &metadata() const
    {
        return m_metadata;
    }
    Metadata::Album &metadata()
    {
        return m_metadata;
    }
    void setMetadata(const Metadata::Album &album); // tracks not on the disc are dropped

    const Metadata::Track &trackMetadata(int number) const;
    Metadata::Track &trackMetadata(int number); // number must be an entry of the disc

    // Overwrite: the candidate replaces the metadata (disc identifiers are kept).
    // KeepExisting: only missing values are filled from the candidate.
    void applyCandidate(const MetadataCandidate &candidate, Metadata::MergePolicy policy = Metadata::MergePolicy::Overwrite);

    // Fills derived values: disc identifiers, "various artists" guess (if not
    // decided yet) and the artist of the hidden track. Called by
    // applyCandidate(); does not confirm the modifications.
    void completeMetadata();

    bool operator==(const CDInfo &other) const;
    bool operator!=(const CDInfo &other) const
    {
        return !(*this == other);
    }

private:
    void pruneMetadata();

    Cdda::Toc m_toc;
    QString m_discUdi;
    QString m_cddbDiscId;
    QString m_musicBrainzDiscId;
    QString m_musicBrainzToc;
    Metadata::Album m_metadata;
    Medium m_medium = Medium::Unknown;
    QString m_mediumDetails;
    std::optional<bool> m_cdg;
};

}
