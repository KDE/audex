/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>
#include <QtGlobal>

namespace Audex::Cdda
{

inline constexpr int SectorBytes = 2352;
inline constexpr int SamplesPerSector = 588; // stereo frames per sector
inline constexpr int BytesPerSample = 4; // one stereo frame: 2 x 16 bit, little endian
inline constexpr int SectorsPerSecond = 75;
inline constexpr int PregapSectors = 150; // absolute MSF 00:02:00 == LBA 0
inline constexpr int C2Bytes = 294; // one error bit per byte of the 2352 byte sector

// Distance between the end of the audio session and the first data track of an
// Enhanced CD / CD-Extra: lead-out (6750) + lead-in (4500) + pregap (150).
inline constexpr int EnhancedCdSessionGap = 11400;

// Absolute MSF (as found in the TOC) to LBA. Minutes >= 90 denote negative
// addresses (lead-in area), see MMC "LBA to MSF translation".
inline constexpr int msfToLba(int min, int sec, int frame)
{
    int lba = (min * 60 + sec) * SectorsPerSecond + frame - PregapSectors;
    if (min >= 90)
        lba -= 450000;
    return lba;
}

// "mm:ss.ff" (75 frames per second)
QString sectorsToMsf(qint64 sectors);

// "m:ss.mmm"
QString sectorsToTime(qint64 sectors);

}
