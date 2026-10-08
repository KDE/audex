/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "core/ripengine.h"
#include "core/toc.h"
#include "encoding/encoder.h"
#include "encoding/tagwriter.h"
#include "online/ctdb.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <functional>
#include <optional>

namespace Audex
{

// Repairs a finished image file with the recovery data of the CUETools
// database. The file is read back, compared with the database entries and,
// if one of them can be restored, written again with the same encoder. The
// result replaces the file only if it matches the entry's CRC.
struct ImageRepairRequest {
    QString path;
    const Encoding::EncoderFactory *encoder = nullptr; // must be able to read the file back
    QVariantMap encoderSettings;
    Encoding::TagInfo tags;
    bool writeTags = true;
    bool keepOriginal = false; // keep the unrepaired file as "<name>.unrepaired.<suffix>"

    Cdda::Toc toc;
    int firstLba = 0; // disc position of the first sample of the image
    QList<Ctdb::Entry> entries;

    // downloads the recovery data; Ctdb::fetchParity() if not set
    std::function<QByteArray(const Ctdb::Entry &entry, QString *error)> fetchParity;
};

struct ImageRepairResult {
    bool repaired = false;
    int corrections = 0; // 16 bit values
    QString originalPath; // keepOriginal
    QStringList log; // rip log section, the first line being its heading
};

ImageRepairResult
repairImage(const ImageRepairRequest &request, const std::function<void(const QString &)> &message = {}, const std::function<bool()> &isCanceled = {});

// "<name>.unrepaired.<suffix>" next to the image
QString unrepairedPath(const QString &path);

// Replaces parts of a finished image with audio read again: the secure
// extraction of the tracks the databases did not confirm after the fast first
// pass. The file is read back and written again with the same encoder; its
// length does not change.
struct ImagePatch {
    qint64 offsetBytes = 0; // position in the PCM of the image
    QString pcmPath; // raw 16 bit stereo PCM that replaces as many bytes
};

struct ImagePatchRequest {
    QString path;
    const Encoding::EncoderFactory *encoder = nullptr; // must be able to read the file back
    QVariantMap encoderSettings;
    Encoding::TagInfo tags;
    bool writeTags = true;
    QList<ImagePatch> patches; // ascending, not overlapping
};

bool patchImage(const ImagePatchRequest &request, QString *error, const std::function<bool()> &isCanceled = {});

// Checksums of a finished image as the extraction computes them (copy CRCs,
// AccurateRip, CTDB, also for other pressings): the image is read back and
// run through the rip engine like a disc, fast mode without offset. Audio
// beyond the image counts as silence. For the rip log after a repair.
struct ImageChecksumRequest {
    QString path;
    const Encoding::EncoderFactory *encoder = nullptr; // must be able to read the file back
    QList<Rip::Segment> segments; // contiguous; the image starts with the first
    Rip::RipOptions options; // of the extraction: shifts and CTDB range are used
};

std::optional<QList<Rip::SegmentResult>> checksumImage(const ImageChecksumRequest &request, QString *error, const std::function<bool()> &isCanceled = {});

}
