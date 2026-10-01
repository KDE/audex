/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>
#include <optional>

#include "core/cdtext.h"
#include "core/sectorreader.h"
#include "metadata/candidate.h"
#include "metadata/cdinfo.h"

class QThreadPool;

namespace Audex
{

inline constexpr auto DemoDriveId = "sim:demo";

struct DriveEntry {
    QString id; // device node or DemoDriveId
    QString displayName;

    bool isSimulated() const
    {
        return id.startsWith(u"sim:");
    }
};

struct DiscReadResult {
    bool ok = false;
    QString error;
    QStringList notes;
    QString driveName;
    bool c2Capable = false;
    bool c2Known = false; // false: the capability page could not be read
    CDInfo info;
    MetadataCandidates discCandidates; // from CD-Text
    bool htoaSilent = false; // hidden track one audio is digital silence
};

// Hidden track one audio is usually a short pregap of digital silence. Reads
// at most 10 s; longer or unreadable ones count as audio.
bool isHtoaSilent(Rip::SectorReader &reader, const Cdda::Toc &toc);

// Blocking: identification, TOC and CD-Text. Run it in a worker thread.
DiscReadResult readDisc(const DriveEntry &drive);

// Opens a sector reader for the drive. For real drives the returned object
// owns the device handle. Blocking; call it in the thread that reads.
struct OpenedReader {
    std::unique_ptr<Rip::SectorReader> reader;
    std::function<void()> release; // unlocks the tray etc.
    QString driveName;
    QString error;
};
OpenedReader openReader(const DriveEntry &drive);

// Drives that stopped answering: a command timed out and the drive did not
// recover. readDisc() and openReader() refuse them until they are removed
// from the system (switched off and on, unplugged). Keyed by device node,
// kept for this session, thread-safe.
void markDriveStalled(const QString &device);
void clearDriveStalled(const QString &device);
bool isDriveStalled(const QString &device);

// What the drive reports about "accurate stream" on its capabilities page,
// noted by readDisc(); nothing if not read (yet). Keyed by device node,
// thread-safe. The drive test measures it (DriveFeatures::accurateStream).
std::optional<bool> reportedAccurateStream(const QString &device);
QThreadPool *driveThreadPool();

// The demo disc: 4 tracks with a hidden track, CD-Text, a drive with read
// offset +6, an audio cache and a slightly noisy sector.
namespace Demo
{
inline constexpr int ReadOffset = 6;
Cdda::Toc toc();
Cdda::CdText cdText();
std::unique_ptr<Rip::SectorReader> reader();
QByteArray expectedAudio(int firstLba, int lastLba, int correctionOffset);
}

}
