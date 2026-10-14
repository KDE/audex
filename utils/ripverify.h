/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "core/toc.h"
#include "encoding/registry.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>
#include <optional>

// Checks a finished rip again, based on its Audex rip log: the files are read
// back, their copy CRCs compared with the log, and AccurateRip and the CUETools
// database asked once more. Used by the application and by audex-cli.
namespace Audex::Verify
{

// ---- the rip log --------------------------------------------------------------

struct LoggedTrack {
    int number = 0; // 0 = hidden track one audio
    QString file; // as written in the log
    int firstLba = 0; // the disc sectors the file holds for this track
    int lastLba = -1;
    std::optional<quint32> copyCrc;
};

struct RipLog {
    QString application; // "Audex 26.11.70"
    Cdda::Toc toc;
    QList<LoggedTrack> tracks; // in the order of the log
};

std::optional<RipLog> parseRipLog(const QString &text, QString *error);

// ---- the check ----------------------------------------------------------------

struct Request {
    QString logPath;
    QString directory; // where the files are if not where the log says; empty: next to the log
    std::shared_ptr<const Encoding::EncoderRegistry> encoders; // decoders by file suffix
    bool online = true; // ask AccurateRip and the CUETools database
    int minConfidence = 2;
};

struct TrackResult {
    int number = 0;
    QString path; // the file read; empty if it was not found
    QString problem; // why the track could not be checked (completely)
    std::optional<quint32> loggedCrc;
    std::optional<quint32> crc; // read now
    int accurateRipConfidence = -1; // -1: not asked (offline, not in the database, hidden track)
    int ctdbConfidence = -1;

    bool changed() const
    {
        return loggedCrc && crc && *loggedCrc != *crc;
    }
};

enum class Verdict {
    Ok, // every file unchanged and (online) every track confirmed by a database
    Unconfirmed, // nothing is wrong, but not every track could be confirmed
    Failed, // a file changed or a database contradicts the rip
    Error // the rip could not be checked (log, missing or unreadable files)
};

struct Result {
    Verdict verdict = Verdict::Error;
    QString error; // Verdict::Error: why
    QString application; // that wrote the log
    QList<TrackResult> tracks;
    bool accurateRipAsked = false;
    bool accurateRipKnown = false; // the disc is in the database
    int accurateRipMismatches = 0;
    bool ctdbAsked = false;
    bool ctdbKnown = false;
    int ctdbMismatches = 0;
    QStringList report;
};

Result verifyRip(const Request &request, const std::function<void(const QString &)> &message = {}, const std::function<bool()> &isCanceled = {});

QString verdictName(Verdict verdict); // "ok", "unconfirmed", "failed", "error"

}
