/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// Q sub-channel: pregaps (index 00), further indexes, ISRC and media catalog
// number. Frames are 12 bytes (10 bytes of data, CRC-16), as READ CD with
// formatted Q sub-channel delivers them.

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

#include <functional>
#include <optional>

#include "core/toc.h"

namespace Audex::Rip
{
class SectorReader;
}

namespace Audex::Cdda
{

inline constexpr int SubQBytes = 12;

struct SubQ {
    quint8 control = 0;
    quint8 adr = 0; // 1 position, 2 media catalog number, 3 ISRC
    int track = 0; // ADR 1
    int index = 0; // ADR 1
    int relative = 0; // ADR 1, frames; counts down inside a pregap
    int absoluteLba = 0; // ADR 1
    QString mcn; // ADR 2
    QString isrc; // ADR 3
};

// Nothing for a CRC mismatch or invalid BCD. A zero CRC is accepted: some
// drives do not deliver it in the formatted Q data.
std::optional<SubQ> parseSubQ(QByteArrayView frame);
QByteArray formatSubQ(const SubQ &q); // with CRC (simulator, tests)

struct SubchannelScan {
    bool supported = false; // the drive delivers Q data
    QMap<int, int> index00; // track -> first sector of its pregap (index 00)
    QMap<int, QList<int>> indexes; // track -> first sectors of index 02, 03, ...
    QMap<int, QString> isrc; // track -> ISRC
    QString mcn;
    QStringList notes;
};

// Binary search on the Q positions around the TOC track starts, plus a short
// scan per track for ISRC/MCN frames. Takes a few seconds per track.
SubchannelScan scanSubchannel(Rip::SectorReader &reader, const Toc &toc, bool isrcMcn, const std::function<bool()> &isCanceled = {});

// Rip log section
QStringList formatSubchannel(const SubchannelScan &scan, const Toc &toc);

}
