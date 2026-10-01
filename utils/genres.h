/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QStringList>

namespace Audex::Genres
{

/**
 * Loads the genre list from AppDataLocation JSON files with fallback to :/genres.json.
 * Duplicates are filtered case-insensitively, and the result is sorted alphabetically.
 */
[[nodiscard]] QStringList presets();

} // namespace Audex::Genres
