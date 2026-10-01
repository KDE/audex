/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// What a drive can do, measured once with a disc in it: read command size,
// accurate stream, audio cache, C2 error pointers, overread and Q sub-channel.
// The read offset is measured by the caller (it needs the AccurateRip
// database, see online/accuraterip.h) and run in between through
// FeatureCallbacks::offset.

#include "core/sectorreader.h"
#include "core/toc.h"

#include <QDateTime>
#include <QString>
#include <QStringList>

#include <functional>

namespace Audex::Rip
{

// Every check can end undecided: the disc in the drive decides what can be
// measured at all.
enum class Feature {
    Unknown,
    Yes,
    No
};

struct FeatureCheck {
    QString id; // burst, stream, cache, c2, overread, subchannel
    QString name;
    Feature verdict = Feature::Unknown;
    QString summary; // the result in one line
    QStringList details; // what was measured
};

struct DriveFeatures {
    QDateTime measured; // invalid: never measured

    int burstSectors = 0; // largest read command that works, 0 = not measured
    Feature accurateStream = Feature::Unknown;
    int jitterSamples = 0; // largest misalignment between two reads
    Feature caching = Feature::Unknown;
    int cacheDefeatReads = 0; // far reads needed to flush the audio cache
    bool c2Supported = false;
    Feature c2Reliable = Feature::Unknown; // the drive flags the errors it makes
    Feature leadIn = Feature::Unknown; // can read in front of the audio area
    Feature leadOut = Feature::Unknown; // ... and behind it
    Feature subchannelQ = Feature::Unknown;
    Feature rwSubchannel = Feature::Unknown; // raw P-W sub-channel: CD+G graphics
    int rwShift = 0; // it belongs to the sector that many sectors later

    QList<FeatureCheck> checks; // in the order they ran, for the dialog and the log
    bool canceled = false;
    bool driveStalled = false; // the drive stopped answering, the remaining checks were skipped
    QString stalledAfter; // id of the check after which it stopped answering
};

struct FeatureCallbacks {
    // a check begins (id and name are set), and has its result
    std::function<void(int index, int total, const FeatureCheck &check)> started;
    std::function<void(const FeatureCheck &check)> finished;
    std::function<bool()> isCanceled;
    // The read offset (it needs the AccurateRip database, see
    // online/accuraterip.h): run as a check of its own, id "offset", before
    // the checks that can hang a drive.
    std::function<FeatureCheck()> offset;
};

// Measures the features with the disc that is in the drive. Blocking and slow
// (about a minute on a real drive), so run it in a worker thread and store the
// result per drive.
DriveFeatures detectDriveFeatures(SectorReader &reader, const Cdda::Toc &toc, const FeatureCallbacks &callbacks = {});

QStringList formatDriveFeatures(const DriveFeatures &features);

}
