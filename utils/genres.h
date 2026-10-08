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
 * Loads the genre list from the genres.json files in AppDataLocation: the
 * user's own (~/.local/share/audex) and the installed one (share/audex).
 * Duplicates are filtered case-insensitively, and the result is sorted
 * alphabetically. Empty if no file is found; that is logged once.
 */
[[nodiscard]] QStringList presets();

} // namespace Audex::Genres
