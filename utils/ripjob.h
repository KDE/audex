/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QThread>

#include <atomic>
#include <mutex>

#include "core/ripengine.h"
#include "core/subchannel.h"
#include "discsource.h"
#include "encoding/registry.h"
#include "metadata/cdinfo.h"

namespace Audex
{

namespace Encoding
{
class EncodedOutputs;
}

struct RipRequest {
    DriveEntry drive;
    CDInfo disc; // TOC and metadata (file names, log)
    QList<int> tracks; // TOC numbers, 0 = hidden track
    Rip::RipOptions options;
    QString outputDirectory;
    QString fileNamePattern = QStringLiteral("{number} - {artist} - {title}");
    QString application = QStringLiteral("Audex");

    // output format; the registry must outlive the job
    std::shared_ptr<const Encoding::EncoderRegistry> encoders;
    QString encoderId = QStringLiteral("wav");
    QVariantMap encoderSettings;
    bool writeTags = true;
    bool embedCover = true;
    int coverMaxSize = 1000;

    // image file: all selected tracks go into one file (segments sharing a
    // path are merged by EncodedOutputs); the cue sheet is written by the GUI
    bool imageFile = false;
    QString imageFileNamePattern = QStringLiteral("{artist} - {album}");

    // look up the disc in the AccurateRip database before the extraction
    // (pressing offsets, keepConfirmedTracks) and verify the tracks afterwards
    bool accurateRipLookup = false;

    // look up the disc in the CUETools database before the extraction
    // (keepConfirmedTracks) and verify the rip afterwards
    bool ctdbLookup = false;

    // Secure mode: read each track once and keep it if AccurateRip or the
    // CUETools database confirms it with at least minConfidence; only the
    // other tracks are read again securely. In an image they replace their
    // first read after the rip (lossless format that can be read back).
    bool keepConfirmedTracks = false;
    int minConfidence = 2;

    // Image file: repair it with the recovery data of the CUETools database
    // when the database contradicts the rip (needs ctdbLookup); optionally
    // keep the unrepaired file as "<name>.unrepaired.<suffix>"
    bool ctdbRepair = false;
    bool ctdbRepairKeepOriginal = false;

    // tag written into tracks flagged with pre-emphasis (value "1"); an image
    // gets it only if all its tracks are flagged. Empty: no tag.
    QString preEmphasisTag;

    // check every track for HDCD while ripping (rip log); tag it (empty: no tag)
    bool detectHdcd = false;
    QString hdcdTag;

    // read pregaps, indexes, ISRC and MCN from the Q sub-channel before the
    // extraction (log, tags, cue sheet)
    bool scanSubchannel = false;

    // collect ISRCs and the MCN as well (needs scanSubchannel)
    bool scanIsrcMcn = false;

    // the scan was wanted, but the drive test found no usable Q sub-channel
    bool subchannelSkipped = false;

    // track files: a track's pre-gap at the start of its own file instead of
    // at the end of the previous one (needs the scan)
    bool pregapsWithTrack = false;

    // after the audio: CD+G graphics of the disc as .cdg files next to the
    // audio files (one per track, one for an image)
    bool readCdg = false;
    // the detection found graphics on the disc; otherwise the rip checks first
    bool cdgDetected = false;
    // wanted, but the drive test found no raw sub-channel
    bool cdgSkipped = false;

    // the drive does not read accurately (see RipRequestBuilder::streamWarning())
    bool inaccurateStream = false;
    bool inaccurateStreamMeasured = false; // by the drive test, with jitterSamples
    int jitterSamples = 0;

    // Write the rip log to this file (empty: no log file). It is written for
    // a canceled or failed rip as well.
    QString logFilePath;

    // replace spaces with underscores in file names (Audex profile option)
    bool underscores = false;

    // pad the track number in file names to two digits (Audex profile option)
    bool twoDigitTrackNumbers = true;

    // Complete output file per track number (absolute or relative to
    // outputDirectory). Tracks listed here ignore the name patterns above.
    QMap<int, QString> filePaths;
};

// Placeholders: {number} {artist} {title} {album} {year}. Characters that are
// not allowed in file names are replaced; "." + suffix is appended.
QString trackFileName(const CDInfo &disc,
                      int trackNumber,
                      const QString &pattern,
                      const QString &suffix = QStringLiteral("wav"),
                      bool underscores = false,
                      bool twoDigitNumber = true);

// Same placeholders for the image file of a whole disc: {title} and {album}
// both mean the album title, {number} is "1".
QString imageFileName(const CDInfo &disc, const QString &pattern, const QString &suffix = QStringLiteral("wav"), bool underscores = false);

struct RipSummary {
    bool completed = false;
    bool canceled = false;
    QString error;
    int tracks = 0;
    int suspiciousPositions = 0;
    int accurateRipMismatches = 0; // tracks the AccurateRip database contradicts
    int ctdbMismatches = 0; // tracks the CUETools database contradicts (0 after a repair)
    int ctdbCorrections = 0; // values the CUETools database repair corrected
    int measuredCacheDefeatReads = 0; // > 0: measured by this rip, to be stored per drive
    QStringList files;
    QString logFile;
    QStringList report;
    Cdda::SubchannelScan subchannel; // scanSubchannel: pregaps etc. for the cue sheet
};

// Runs one extraction in its own thread. Signals arrive in the thread of the
// RipJob object (normally the GUI thread).
class RipJob : public QObject
{
    Q_OBJECT

public:
    explicit RipJob(RipRequest request, QObject *parent = nullptr);
    ~RipJob() override; // cancels and waits

    void start();
    void cancel();
    bool isRunning() const;
    bool wait(int timeoutMs = -1);

Q_SIGNALS:
    // trackSectors: sectors of trackNumber delivered by its running extraction
    void progress(qint64 doneSectors, qint64 totalSectors, int trackNumber, qint64 trackSectors);
    void trackStatus(int trackNumber, int status); // Rip::SegmentStatus
    void message(int level, const QString &text); // Rip::LogLevel
    void finished(const Audex::RipSummary &summary);

private:
    void run(); // worker thread

    RipRequest m_request;
    QPointer<QThread> m_thread;
    std::atomic_bool m_cancel{false};
    std::mutex m_outputsMutex;
    Encoding::EncodedOutputs *m_outputs = nullptr; // set while run() may block in it
};

}

Q_DECLARE_METATYPE(Audex::RipSummary)
