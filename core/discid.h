/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QList>
#include <QString>

#include <optional>

#include "toc.h"

// Disc identifiers. All algorithms work on absolute frame offsets
// (LBA + 150), which is what the TOC stores on disc.

namespace Audex::Cdda
{

// ---- CDDB / freedb / gnudb ---------------------------------------------------
// Uses all tracks (including data tracks) and the lead-out of the disc.

quint32 cddbDiscId(const Toc &toc);
QString cddbDiscIdString(const Toc &toc); // 8 lowercase hex digits

// ---- MusicBrainz ---------------------------------------------------------------
// Uses the audio part only: trailing data tracks (Enhanced CD) are dropped and
// the lead-out is taken as "start of the first dropped data track - 11400",
// exactly like libdiscid does. Leading data tracks (Mixed Mode) are kept.

struct MusicBrainzToc {
    int firstTrack = 0;
    int lastTrack = 0;
    int leadOut = 0; // absolute frames
    QList<int> offsets; // absolute frames, one per track firstTrack..lastTrack
};

std::optional<MusicBrainzToc> musicBrainzToc(const Toc &toc);
QString musicBrainzDiscId(const MusicBrainzToc &toc);
QString musicBrainzDiscId(const Toc &toc); // empty if the disc has no audio tracks

// "first last leadout offset1 offset2 ..." as used by libdiscid and by the
// MusicBrainz web service ("toc" parameter, with '+' as separator).
QString musicBrainzTocString(const MusicBrainzToc &toc, QChar separator = u' ');
QString musicBrainzTocString(const Toc &toc, QChar separator = u' ');

// Builds an audio-only TOC from such a string (tests, CLI without a drive).
std::optional<Toc> tocFromMusicBrainzTocString(const QString &text);

}
