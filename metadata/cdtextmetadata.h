/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "candidate.h"
#include "cdinfo.h"
#include "core/cdtext.h"

namespace Audex
{

// Maps one CD-Text block onto album metadata (tracks keyed by TOC number).
Metadata::Album albumFromCdTextBlock(const Cdda::CdTextBlock &block, const CDInfo &disc);

// Candidate from the preferred language block (English, otherwise block 0).
// Other language blocks become additional candidates with a lower score.
MetadataCandidates candidatesFromCdText(const Cdda::CdText &cdText, const CDInfo &disc, quint8 preferredLanguage = 0x09);

}
