/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <functional>
#include <optional>

#include "core/ripengine.h"
#include "core/toc.h"

class QNetworkAccessManager;
class QNetworkReply;

// Client for the AccurateRip database (https://www.accuraterip.com).
// There is no official public API; the URL scheme, the disc identification
// and the binary formats used here match what other open source rippers
// (whipper, CUETools) implement and what the AccurateRip server accepts.
namespace Audex::AccurateRip
{

// ---- disc identification ---------------------------------------------------

struct DiscIds {
    quint32 id1 = 0;
    quint32 id2 = 0;
    quint32 cddbId = 0;
    int audioTracks = 0;
};

// The two AccurateRip disc IDs and the CDDB disc ID, computed from the TOC.
// Audio tracks are summed by their index-01 LBA; data tracks do not enter the
// sums, but the lead-out of the last session does. An empty TOC yields zeroed
// ids.
DiscIds discIds(const Cdda::Toc &toc);

// https://www.accuraterip.com/accuraterip/x/x/x/dBAR-NNN-xxxxxxxx-xxxxxxxx-xxxxxxxx.bin
QUrl databaseUrl(const DiscIds &ids);

// Machine-readable drive offset list (the same data driveoffsets.htm renders).
QUrl driveOffsetsUrl();

// ---- database responses ----------------------------------------------------

struct TrackEntry {
    quint8 confidence = 0; // how often this checksum was submitted
    quint32 checksum = 0; // AccurateRip v1 or v2 checksum (submission dependent)
    quint32 frame450Checksum = 0;
};

// One submission for the disc; a .bin reply usually holds several, one after
// another. tracks is indexed by audio position (track 1 of the disc is index
// 0); hidden track one audio is not covered by the database.
struct Response {
    int trackCount = 0;
    quint32 id1 = 0;
    quint32 id2 = 0;
    quint32 cddbId = 0;
    QList<TrackEntry> tracks;
};

// Parses a dBAR reply: 13 byte header (track count byte + 3 little endian
// quint32 disc ids), then trackCount records of 9 bytes (confidence byte +
// checksum + frame 450 checksum). Truncated input yields the complete
// responses only.
QList<Response> parseResponses(const QByteArray &data);

// ---- drive offsets ---------------------------------------------------------

struct DriveOffset {
    QString name; // as submitted, e.g. "PLEXTOR  - DVDR   PX-760A"
    int offset = 0; // samples, AccurateRip convention
};

// DriveOffsets.bin: records of 69 bytes (little endian qint16 offset, 33 byte
// zero padded ASCII name, 34 bytes of flags/statistics we do not need).
QList<DriveOffset> parseDriveOffsets(const QByteArray &data);

// Finds the entry for a drive name (e.g. the vendor/product string of the
// drive). Matching is done on the normalized names (lower case, letters and
// digits only): exact match first, then containment in either direction,
// longest candidate name wins. Returns nothing if there is no plausible match.
std::optional<DriveOffset> findDriveOffset(const QList<DriveOffset> &offsets, const QString &driveName);

// ---- matching ----------------------------------------------------------------

// Highest confidence of a database entry for the track at audioIndex (0 based)
// that matches v1 or v2; 0 if none does.
int matchConfidence(const QList<Response> &responses, int audioIndex, quint32 v1, quint32 v2);

struct PressingOffset {
    int shift = 0; // database sample i == our sample i + shift
    int tracks = 0; // probed tracks whose frame 450 checksum matched
    int confidence = 0; // sum of the matching confidences
};

// Shifts at which the frame 450 checksums of probed tracks match database
// entries, best first. checksums: audio index -> frame450Checksums() of that track.
QList<PressingOffset> matchFrame450(const QList<Response> &responses, const QMap<int, QList<quint32>> &checksums, int range = Rip::AccurateRipShiftRange);

// Reads the surroundings of sector 450 of up to maxTracks audio tracks with
// the given read offset; result keyed by audio index (see matchFrame450()).
QMap<int, QList<quint32>> probeFrame450(Rip::SectorReader &reader,
                                        const Cdda::Toc &toc,
                                        int readOffset,
                                        int maxTracks = 3,
                                        const std::function<bool()> &isCanceled = {});

struct OffsetDetection {
    bool found = false;
    int offset = 0; // read offset correction, AccurateRip convention
    int tracks = 0; // probed tracks that agree
    int confidence = 0; // database confidence of the confirming track
    QString details;
};

// Finds the drive's read offset with any disc in the database: frame 450
// search on raw reads, confirmed by extracting one whole track.
OffsetDetection detectReadOffset(Rip::SectorReader &reader, const Cdda::Toc &toc, const QList<Response> &responses, const std::function<bool()> &isCanceled = {});

// ---- verification ----------------------------------------------------------

// Compares the AccurateRip checksums computed while ripping (segments with
// the accurateRip flag set) against the database responses and formats the
// outcome as log lines. An empty response list means the disc is not in the
// database. The first line is a section heading; used in the rip report.
// inaccurateTracks receives the number of tracks the database contradicts.
QStringList formatVerification(const QList<Response> &responses,
                               const QList<Rip::SegmentResult> &segments,
                               const Cdda::Toc &toc,
                               int *inaccurateTracks = nullptr);

// Synchronous lookup of the database entry for a disc, for worker threads
// (RipJob). Sets notFound when the server answered 404 and error on network
// or protocol failures; both are not fatal for a rip.
QList<Response> lookupDiscEntry(const Cdda::Toc &toc,
                                QString *error,
                                bool *notFound = nullptr,
                                int timeoutMs = 30000,
                                const std::function<bool()> &isCanceled = {});

// ---- asynchronous access (GUI thread) --------------------------------------

class Client : public QObject
{
    Q_OBJECT

public:
    explicit Client(QObject *parent = nullptr);
    ~Client() override;

    bool isBusy() const;

    void fetchDriveOffsets();

    void cancel();

Q_SIGNALS:
    void driveOffsetsFetched(const QList<Audex::AccurateRip::DriveOffset> &offsets);
    void failed(const QString &message);

private:
    void get(const QUrl &url);
    void replyFinished();

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;

    enum class Pending { None, DriveOffsets };
    Pending m_pending = Pending::None;
};

}
