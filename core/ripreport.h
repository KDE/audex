/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QMap>
#include <QStringList>

#include "ripengine.h"
#include "toc.h"

namespace Audex::Rip
{

struct ReportContext {
    QString application;
    QString drive;
    QString device;
    QString medium; // "pressed CD", "CD-R (lead-in 97:24:01, ...)"
    QString gapHandling; // track files: where the pre-gaps went (empty for an image)
    Rip::RipOptions options;
    Cdda::Toc toc;
    QStringList fileNames; // per segment
    QStringList driveNotes; // capability warnings etc.
    QString encoder; // e.g. "FLAC (libFLAC 1.4.3), compression=5"
    QStringList outputNotes; // encoder and tagging warnings
    QStringList accurateRip; // AccurateRip verification section, empty = not performed
    QStringList ctdb; // CUETools database section, empty = not performed
    QStringList subchannel; // pregaps/indexes/ISRC section, empty = not read
    // image repaired with the CUETools database and read back: the checksums
    // are those of the repaired image
    bool repaired = false;
    QMap<int, quint32> crcBeforeRepair; // segment index -> copy CRC before the repair
    QMap<int, QStringList> trackNotes; // segment index -> further lines (HDCD ...)
};

QString modeDescription(const Rip::RipOptions &options);
QStringList formatReport(const ReportContext &context, const Rip::RipResult &result);

}
