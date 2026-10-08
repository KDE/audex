/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// CD+G: graphics in the R-W sub-channels of an audio CD (karaoke discs).
//
// Every sector carries 96 bytes of sub-channel data, one bit of each byte per
// channel (P in bit 7, Q in bit 6, R-W in bits 5..0). The R-W symbols form
// four packs of 24 per sector, 300 per second. On the disc the packs are
// scrambled (three symbol pairs swapped) and interleaved: symbol i of pack x
// travels in pack x + (i mod 8). Drives deliver the raw data that way; a few
// de-interleave it themselves.
//
// A .cdg file holds the de-interleaved packs, 96 bytes per sector, as players
// (FFmpeg, VLC, karaoke software) expect them next to the audio file.

#include <QByteArray>
#include <QString>

#include <functional>

#include "core/toc.h"

namespace Audex::Rip
{
class SectorReader;
}

namespace Audex::Cdda
{

inline constexpr int SubcodeBytes = 96; // raw P-W per sector
inline constexpr int CdgPackBytes = 24;
inline constexpr int CdgPacksPerSector = 4;

// The Q channel of a raw P-W block: a 12 byte frame for parseSubQ()
QByteArray qFrameFromSubcode(QByteArrayView block);

// The R-W symbols (bits 5..0) of raw P-W blocks, same length
QByteArray rwFromSubcode(QByteArrayView blocks);

// Packs as on the disc -> as written (and back, for the simulator). The
// packs at the end that would need later packs get zeros there.
QByteArray deinterleaveCdg(QByteArrayView packs);
QByteArray interleaveCdg(QByteArrayView packs);

// Packs of the TV graphics (and extended graphics) mode with a known instruction
int cdgGraphicsPacks(QByteArrayView packs);

// A few seconds of sub-channel at three places: does the disc carry CD+G?
// Sent as probes (short timeout), for drives that cannot do it.
bool detectCdg(Rip::SectorReader &reader, const Toc &toc, const std::function<bool()> &isCanceled = {});

struct CdgExtraction {
    bool found = false; // the range carries CD+G graphics
    QByteArray packs; // 96 bytes per sector of [firstLba, lastLba]: a .cdg file
    int graphicsPacks = 0;
    int shift = 0; // the drive delivers the sub-channel of sector n + shift with sector n
    bool shiftMeasured = false;
    bool deinterleavedByDrive = false;
    int rereadSectors = 0; // the first two reads differed
    int unresolvedSectors = 0; // no two reads agreed
    QString error; // the sub-channel could not be read (completely)
};

// Where extractCdg() is: both reads cover the whole range, which takes
// minutes on drives that deliver the raw sub-channel slowly.
struct CdgProgress {
    int pass = 1; // 1, 2: the reads of the whole range; 3: differing sectors read again
    int lba = 0; // read position
    qint64 done = 0; // sectors read in all passes so far
    qint64 total = 0; // two passes, plus the sectors read again once they are known
};

// Reads the sub-channel of [firstLba, lastLba] twice at full speed, reads
// sectors again whose R-W data differed until two reads agree, corrects the
// drive's sub-channel shift (measured with the Q channel of the same data) and
// de-interleaves the packs if the drive did not.
CdgExtraction extractCdg(Rip::SectorReader &reader,
                         const Toc &toc,
                         int firstLba,
                         int lastLba,
                         const std::function<void(const QString &)> &message = {},
                         const std::function<bool()> &isCanceled = {},
                         const std::function<void(const CdgProgress &)> &progress = {});

}
