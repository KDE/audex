/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include "cdda.h"

namespace Audex::Cdda
{

// All sector addresses are LBA (LBA 0 == absolute MSF 00:02:00).
struct Track {
    int number = 0;
    int session = 1;
    int firstLba = 0; // index 01
    int lastLba = -1; // inclusive
    bool audio = true;
    bool preEmphasis = false;
    bool copyPermitted = false;
    bool fourChannel = false;

    int sectorCount() const
    {
        return lastLba - firstLba + 1;
    }

    bool operator==(const Track &other) const = default;
};

class Toc
{
public:
    QList<Track> tracks; // sorted by track number
    int leadOutLba = 0; // lead-out of the last session
    QString source; // how the TOC was obtained, for the log

    bool isEmpty() const
    {
        return tracks.isEmpty();
    }

    // Structural sanity check: sorted, non-overlapping, within lead-out.
    bool isValid(QString *error = nullptr) const;

    const Track *track(int number) const;

    QList<int> audioTrackNumbers() const;
    int firstAudioTrackNumber() const; // 0 if there is none
    int lastAudioTrackNumber() const; // 0 if there is none

    // Area in which audio sectors can be read without overreading:
    // [audioStartLba(), audioEndLba()) (end exclusive).
    // For an Enhanced CD this ends at the lead-out of the audio session,
    // for a Mixed Mode CD it starts at the first audio track.
    int audioStartLba() const;
    int audioEndLba() const;

    // Hidden Track One Audio: audio before index 01 of track 1.
    int htoaSectorCount() const;

    QStringList describe() const;

    bool operator==(const Toc &other) const = default;
};

}
