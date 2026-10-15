/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "metadata/metadata.h"
#include "online/responsecache.h"

#include <optional>

namespace Audex
{

// The cover the user chose for a disc (from a file or from the Cover Art
// Archive), kept in the cache under the MusicBrainz disc ID. It expires and
// is cleared with the rest of the cache.
namespace ChosenCovers
{

inline constexpr auto Service = "local-covers";

std::optional<Metadata::CoverArt> find(const ResponseCache &cache, const QString &discId);
void keep(ResponseCache &cache, const QString &discId, const Metadata::CoverArt &cover);
void forget(ResponseCache &cache, const QString &discId);

}

}
