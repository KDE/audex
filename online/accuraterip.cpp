/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "accuraterip.h"
#include "blockingfetch.h"

#include "core/discid.h"

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Audex::AccurateRip
{

namespace
{

const char baseUrl[] = "https://www.accuraterip.com/accuraterip/";

quint32 readLE32(const QByteArray &data, qsizetype pos)
{
    const auto *p = reinterpret_cast<const uchar *>(data.constData() + pos);
    return quint32(p[0]) | (quint32(p[1]) << 8) | (quint32(p[2]) << 16) | (quint32(p[3]) << 24);
}

qint16 readLE16(const QByteArray &data, qsizetype pos)
{
    const auto *p = reinterpret_cast<const uchar *>(data.constData() + pos);
    return qint16(quint16(p[0]) | (quint16(p[1]) << 8));
}

QString hex8(quint32 v)
{
    return u"%1"_s.arg(v, 8, 16, QLatin1Char('0')).toUpper();
}

QString normalized(const QString &name)
{
    QString n;
    n.reserve(name.size());
    for (const QChar c : name)
        if (c.isLetterOrNumber())
            n += c.toLower();
    return n;
}

} // namespace

// ---- disc identification ---------------------------------------------------

DiscIds discIds(const Cdda::Toc &toc)
{
    DiscIds ids;
    if (toc.tracks.isEmpty())
        return ids;

    const QList<int> audio = toc.audioTrackNumbers();
    ids.audioTracks = int(audio.size());
    ids.cddbId = Cdda::cddbDiscId(toc);

    quint64 id1 = 0;
    quint64 id2 = 0;
    quint64 position = 0; // audio position, not the TOC number (Mixed Mode CDs)
    for (int number : audio) {
        const Cdda::Track *t = toc.track(number);
        if (!t)
            continue;
        const quint64 offset = quint64(std::max(t->firstLba, 0));
        id1 += offset;
        id2 += std::max<quint64>(offset, 1) * ++position;
    }
    // lead-out of the last session, multiplied by the audio track count + 1
    const quint64 leadOut = quint64(std::max(toc.leadOutLba, 0));
    id1 += leadOut;
    id2 += std::max<quint64>(leadOut, 1) * quint64(ids.audioTracks + 1);

    ids.id1 = quint32(id1 & 0xffffffffULL);
    ids.id2 = quint32(id2 & 0xffffffffULL);
    return ids;
}

QUrl databaseUrl(const DiscIds &ids)
{
    const QString id1 = u"%1"_s.arg(ids.id1, 8, 16, QLatin1Char('0'));
    return QUrl(u"%1%2/%3/%4/dBAR-%5-%6-%7-%8.bin"_s.arg(QLatin1String(baseUrl), id1.at(7), id1.at(6), id1.at(5))
                    .arg(ids.audioTracks, 3, 10, QLatin1Char('0'))
                    .arg(id1)
                    .arg(ids.id2, 8, 16, QLatin1Char('0'))
                    .arg(ids.cddbId, 8, 16, QLatin1Char('0')));
}

QUrl driveOffsetsUrl()
{
    return QUrl(u"%1DriveOffsets.bin"_s.arg(QLatin1String(baseUrl)));
}

// ---- database responses ----------------------------------------------------

QList<Response> parseResponses(const QByteArray &data)
{
    QList<Response> responses;
    qsizetype pos = 0;
    while (pos + 13 <= data.size()) {
        Response r;
        r.trackCount = int(quint8(data.at(pos)));
        if (r.trackCount == 0 || pos + 13 + qsizetype(r.trackCount) * 9 > data.size())
            break;
        r.id1 = readLE32(data, pos + 1);
        r.id2 = readLE32(data, pos + 5);
        r.cddbId = readLE32(data, pos + 9);
        pos += 13;
        for (int i = 0; i < r.trackCount; ++i) {
            TrackEntry e;
            e.confidence = quint8(data.at(pos));
            e.checksum = readLE32(data, pos + 1);
            e.frame450Checksum = readLE32(data, pos + 5);
            r.tracks.append(e);
            pos += 9;
        }
        responses.append(r);
    }
    return responses;
}

// ---- drive offsets ---------------------------------------------------------

QList<DriveOffset> parseDriveOffsets(const QByteArray &data)
{
    QList<DriveOffset> offsets;
    constexpr qsizetype recordSize = 2 + 33 + 34;
    for (qsizetype pos = 0; pos + recordSize <= data.size(); pos += recordSize) {
        DriveOffset d;
        d.offset = int(readLE16(data, pos));
        QByteArray name = data.mid(pos + 2, 33);
        const qsizetype nul = name.indexOf('\0');
        if (nul >= 0)
            name.truncate(nul);
        d.name = QString::fromLatin1(name).trimmed();
        offsets.append(d);
    }
    return offsets;
}

std::optional<DriveOffset> findDriveOffset(const QList<DriveOffset> &offsets, const QString &driveName)
{
    const QString needle = normalized(driveName);
    if (needle.isEmpty())
        return std::nullopt;

    const DriveOffset *best = nullptr;
    bool exact = false;
    for (const DriveOffset &d : offsets) {
        const QString candidate = normalized(d.name);
        // short generic names ("- ASUS DRW-24F1" is fine, "ATA - ..." stubs
        // are not) must not match by containment
        if (candidate.size() < 6)
            continue;
        if (candidate == needle) {
            if (!exact || (best && candidate.size() > normalized(best->name).size())) {
                best = &d;
                exact = true;
            }
        } else if (!exact && (candidate.contains(needle) || needle.contains(candidate))) {
            if (!best || candidate.size() > normalized(best->name).size())
                best = &d;
        }
    }
    if (!best)
        return std::nullopt;
    return *best;
}

// ---- matching ----------------------------------------------------------------

int matchConfidence(const QList<Response> &responses, int audioIndex, quint32 v1, quint32 v2)
{
    int confidence = 0;
    for (const Response &r : responses) {
        if (audioIndex < 0 || audioIndex >= r.tracks.size())
            continue;
        const TrackEntry &e = r.tracks.at(audioIndex);
        if (e.checksum != 0 && (e.checksum == v1 || e.checksum == v2))
            confidence = std::max(confidence, int(e.confidence));
    }
    return confidence;
}

QList<PressingOffset> matchFrame450(const QList<Response> &responses, const QMap<int, QList<quint32>> &checksums, int range)
{
    QMap<int, PressingOffset> byShift;
    for (auto it = checksums.cbegin(); it != checksums.cend(); ++it) {
        const QList<quint32> &crcs = it.value();
        if (crcs.size() != 2 * range + 1)
            continue;
        QMap<int, int> best; // shift -> best confidence for this track
        for (const Response &r : responses) {
            if (it.key() >= r.tracks.size())
                continue;
            const TrackEntry &e = r.tracks.at(it.key());
            if (e.frame450Checksum == 0)
                continue;
            for (int i = 0; i < crcs.size(); ++i)
                if (crcs.at(i) == e.frame450Checksum)
                    best[i - range] = std::max(best.value(i - range), int(e.confidence));
        }
        for (auto b = best.cbegin(); b != best.cend(); ++b) {
            PressingOffset &p = byShift[b.key()];
            p.shift = b.key();
            ++p.tracks;
            p.confidence += b.value();
        }
    }
    QList<PressingOffset> result = byShift.values();
    std::sort(result.begin(), result.end(), [](const PressingOffset &a, const PressingOffset &b) {
        return a.tracks != b.tracks ? a.tracks > b.tracks : a.confidence > b.confidence;
    });
    return result;
}

QMap<int, QList<quint32>> probeFrame450(Rip::SectorReader &reader, const Cdda::Toc &toc, int readOffset, int maxTracks, const std::function<bool()> &isCanceled)
{
    constexpr int range = Rip::AccurateRipShiftRange;
    constexpr int firstSector = 445; // sectors 445..455 hold frame 450 +- range
    constexpr int sectors = 11;

    const QList<int> audio = toc.audioTrackNumbers();
    QList<int> candidates; // audio indices of tracks long enough
    for (int i = 0; i < audio.size(); ++i)
        if (const Cdda::Track *t = toc.track(audio.at(i)); t && t->sectorCount() > firstSector + sectors)
            candidates.append(i);
    QList<int> chosen;
    for (int k = 0; k < std::min<int>(maxTracks, candidates.size()); ++k)
        chosen.append(candidates.at(int(qint64(k) * candidates.size() / std::min<int>(maxTracks, candidates.size()))));

    QMap<int, QList<quint32>> result;
    for (int index : std::as_const(chosen)) {
        const Cdda::Track *t = toc.track(audio.at(index));
        Rip::Segment segment;
        segment.trackNumber = t->number;
        segment.firstLba = t->firstLba + firstSector;
        segment.lastLba = segment.firstLba + sectors - 1;
        segment.accurateRip = false;

        Rip::RipOptions options;
        options.mode = Rip::ReadMode::Fast;
        options.readOffset = readOffset;
        options.cacheDefeat = false;
        QByteArray pcm;
        Rip::RipCallbacks callbacks;
        callbacks.write = [&pcm](int, QByteArrayView data) {
            pcm.append(data.constData(), data.size());
            return true;
        };
        callbacks.isCanceled = isCanceled;
        Rip::RipEngine engine(reader, {toc.audioStartLba(), toc.audioEndLba()}, options, callbacks);
        if (!engine.run({segment}).completed)
            break;
        // sample 450 * 588 - range of the track is sample 1 of sector 445
        result.insert(index, Rip::frame450Checksums(QByteArrayView(pcm).sliced(Cdda::BytesPerSample), range));
    }
    return result;
}

OffsetDetection detectReadOffset(Rip::SectorReader &reader, const Cdda::Toc &toc, const QList<Response> &responses, const std::function<bool()> &isCanceled)
{
    OffsetDetection d;
    const QMap<int, QList<quint32>> probed = probeFrame450(reader, toc, 0, 5, isCanceled);
    const QList<PressingOffset> matches = matchFrame450(responses, probed);
    if (probed.isEmpty() || matches.isEmpty()) {
        d.details = u"No frame 450 match in %1 probed track(s)."_s.arg(probed.size());
        return d;
    }
    const PressingOffset best = matches.first();
    d.offset = best.shift;
    d.tracks = best.tracks;
    if (best.tracks < std::min<int>(2, probed.size())) {
        d.details = u"Only %1 of %2 probed tracks match at offset %3."_s.arg(best.tracks).arg(probed.size()).arg(best.shift);
        return d;
    }

    // confirm with the shortest probed track: extract it and compare its checksums
    const QList<int> audio = toc.audioTrackNumbers();
    int confirmIndex = probed.firstKey();
    for (int index : probed.keys())
        if (toc.track(audio.at(index))->sectorCount() < toc.track(audio.at(confirmIndex))->sectorCount())
            confirmIndex = index;
    const Cdda::Track *t = toc.track(audio.at(confirmIndex));
    Rip::Segment segment;
    segment.trackNumber = t->number;
    segment.firstLba = t->firstLba;
    segment.lastLba = t->lastLba;
    segment.accurateRipFirst = confirmIndex == 0;
    segment.accurateRipLast = confirmIndex == audio.size() - 1;

    Rip::RipOptions options;
    options.mode = Rip::ReadMode::Fast;
    options.readOffset = best.shift;
    options.cacheDefeat = false;
    Rip::RipCallbacks callbacks;
    callbacks.isCanceled = isCanceled;
    Rip::RipEngine engine(reader, {toc.audioStartLba(), toc.audioEndLba()}, options, callbacks);
    const Rip::RipResult r = engine.run({segment});
    if (!r.completed) {
        d.details = u"Extracting track %1 failed: %2"_s.arg(t->number).arg(r.error);
        return d;
    }
    d.confidence = matchConfidence(responses, confirmIndex, r.segments.first().accurateRipV1, r.segments.first().accurateRipV2);
    d.found = d.confidence > 0;
    d.details = d.found
        ? u"Offset %1: %2 of %3 probed tracks match, track %4 accurately ripped (confidence %5)."_s.arg(best.shift)
              .arg(best.tracks)
              .arg(probed.size())
              .arg(t->number)
              .arg(d.confidence)
        : u"Offset %1 matched frame 450 in %2 track(s), but track %3 does not match the database."_s.arg(best.shift).arg(best.tracks).arg(t->number);
    return d;
}

// ---- verification ----------------------------------------------------------

QStringList formatVerification(const QList<Response> &responses, const QList<Rip::SegmentResult> &segments, const Cdda::Toc &toc, int *inaccurateTracks)
{
    if (inaccurateTracks)
        *inaccurateTracks = 0;

    const QList<int> audio = toc.audioTrackNumbers();

    QStringList l;
    l << u"AccurateRip verification"_s;
    l << QString();

    if (responses.isEmpty()) {
        l << u"     Disc not found in the AccurateRip database."_s;
        return l;
    }

    int accurate = 0;
    int inaccurate = 0;
    int unknown = 0;

    for (const Rip::SegmentResult &s : segments) {
        if (!s.segment.accurateRip)
            continue;
        const int index = audio.indexOf(s.segment.trackNumber); // audio position - 1
        if (index < 0)
            continue;

        int submissions = 0;
        int maxConfidence = 0;
        quint32 maxConfidenceCrc = 0;
        for (const Response &r : responses) {
            if (index >= r.tracks.size())
                continue;
            const TrackEntry &e = r.tracks.at(index);
            submissions += e.confidence;
            if (int(e.confidence) > maxConfidence) {
                maxConfidence = int(e.confidence);
                maxConfidenceCrc = e.checksum;
            }
        }
        const int confidenceV1 = matchConfidence(responses, index, s.accurateRipV1, 0);
        const int confidenceV2 = matchConfidence(responses, index, 0, s.accurateRipV2);
        int shiftedConfidence = 0;
        int shift = 0;
        for (const Rip::ShiftedChecksums &c : s.accurateRipShifted) {
            const int confidence = matchConfidence(responses, index, c.v1, c.v2);
            if (confidence > shiftedConfidence) {
                shiftedConfidence = confidence;
                shift = c.shift;
            }
        }

        const QString track = u"Track %1"_s.arg(s.segment.trackNumber, 2, 10, QLatin1Char('0'));
        if (confidenceV1 > 0 || confidenceV2 > 0) {
            ++accurate;
            const int confidence = std::max(confidenceV1, confidenceV2);
            const QString version = confidenceV2 >= confidenceV1 ? u"v2"_s : u"v1"_s;
            l << u"     %1: accurately ripped (%2, confidence %3 of %4 submissions)"_s.arg(track, version).arg(confidence).arg(submissions);
        } else if (shiftedConfidence > 0) {
            ++accurate;
            l << u"     %1: accurately ripped, other pressing with offset %2 (confidence %3 of %4 submissions)"_s.arg(track)
                     .arg(shift > 0 ? u"+%1"_s.arg(shift) : QString::number(shift))
                     .arg(shiftedConfidence)
                     .arg(submissions);
        } else if (maxConfidence > 0) {
            ++inaccurate;
            l << u"     %1: NOT accurately ripped (database expects %2, confidence %3)"_s.arg(track, hex8(maxConfidenceCrc)).arg(maxConfidence);
        } else {
            ++unknown;
            l << u"     %1: not in database"_s.arg(track);
        }
    }

    if (inaccurateTracks)
        *inaccurateTracks = inaccurate;

    l << QString();
    if (inaccurate > 0)
        l << u"Warning: %1 track(s) could NOT be verified as accurate."_s.arg(inaccurate);
    else if (accurate > 0 && unknown == 0)
        l << u"All tracks accurately ripped."_s;
    else if (accurate > 0)
        l << u"%1 track(s) accurately ripped, %2 not present in the database."_s.arg(accurate).arg(unknown);
    else
        l << u"No track is present in the AccurateRip database."_s;

    return l;
}

QList<Response> lookupDiscEntry(const Cdda::Toc &toc, QString *error, bool *notFound, int timeoutMs, const std::function<bool()> &isCanceled)
{
    const QByteArray data = Online::fetchBlocking(databaseUrl(discIds(toc)), error, notFound, timeoutMs, isCanceled);
    if (data.isEmpty())
        return {};
    return parseResponses(data);
}

// ---- asynchronous access ---------------------------------------------------

Client::Client(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

Client::~Client()
{
    cancel();
}

bool Client::isBusy() const
{
    return m_pending != Pending::None;
}

void Client::get(const QUrl &url)
{
    cancel();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, Online::userAgent());
    request.setTransferTimeout(30000);
    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::finished, this, &Client::replyFinished);
}

void Client::fetchDriveOffsets()
{
    get(driveOffsetsUrl());
    m_pending = Pending::DriveOffsets; // get() calls cancel(), which resets m_pending
}

void Client::cancel()
{
    if (m_reply) {
        disconnect(m_reply, nullptr, this, nullptr);
        m_reply->abort(); // closes the connection now, not only when the reply is deleted
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_pending = Pending::None;
}

void Client::replyFinished()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    const Pending pending = m_pending;
    m_pending = Pending::None;
    if (!reply)
        return;
    reply->deleteLater();

    if (reply->error() == QNetworkReply::ContentNotFoundError) {
        Q_EMIT failed(QCoreApplication::translate("Audex::AccurateRip", "Not found on the server (404)."));
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        Q_EMIT failed(reply->errorString());
        return;
    }

    const QByteArray data = reply->readAll();
    if (pending == Pending::DriveOffsets)
        Q_EMIT driveOffsetsFetched(parseDriveOffsets(data));
}

}
