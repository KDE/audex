/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// MusicBrainz web service (WS/2, JSON) - request building and response parsing.
// Pure functions, no network access, so they can be tested with fixtures.

#include <QByteArray>
#include <QJsonArray>
#include <QUrl>

#include "candidate.h"
#include "cdinfo.h"

namespace Audex::MusicBrainz
{

inline constexpr auto DefaultServer = "https://musicbrainz.org";
inline constexpr auto IncludeParameter = "artist-credits+recordings+isrcs+release-groups";
inline constexpr auto VariousArtistsId = "89ad4ac3-39f7-470e-963a-56509c546377";

// /ws/2/discid/<id>?toc=...&cdstubs=no&inc=...&fmt=json
// The toc parameter enables the fuzzy lookup if the disc id is unknown.
QUrl discIdLookupUrl(const CDInfo &disc, const QUrl &server = QUrl(QString::fromLatin1(DefaultServer)));

struct ParseResult {
    MetadataCandidates candidates;
    QString error; // malformed response
};

// Scores: 100 = medium carries this disc id, 90 = exact disc id lookup but the
// medium is only matched by track count, 70 = fuzzy TOC match.
ParseResult parseDiscIdResponse(const QByteArray &json, const CDInfo &disc);

// "Artist A feat. Artist B"
QString artistCreditString(const QJsonArray &credit);

}
