/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ctdb.h"

#include "core/checksums.h"
#include "online/blockingfetch.h"

#include <QCryptographicHash>
#include <QMap>
#include <QUrlQuery>
#include <QXmlStreamReader>
#include <QtEndian>

#include <algorithm>
#include <optional>

using namespace Qt::StringLiterals;

namespace Audex::Ctdb
{

namespace
{

const char server[] = "http://db.cuetools.net";

QString hex8(quint32 v)
{
    return u"%1"_s.arg(v, 8, 16, QLatin1Char('0'));
}

// sectors from index 01 of the first audio track to the end of the last one
int audioSectors(const Cdda::Toc &toc)
{
    const Cdda::Track *first = toc.track(toc.firstAudioTrackNumber());
    const Cdda::Track *last = toc.track(toc.lastAudioTrackNumber());
    return first && last ? last->lastLba + 1 - first->firstLba : 0;
}

// the same for a database TOC string; -1 if it cannot be parsed
int audioSectors(const QString &toc)
{
    const QStringList ids = toc.split(u':');
    const qsizetype tracks = ids.size() - 1;
    if (tracks < 1)
        return -1;
    const auto isData = [&](qsizetype i) {
        return ids.at(i).startsWith(u'-');
    };
    const auto start = [&](qsizetype i) {
        return QStringView(ids.at(i)).mid(isData(i) ? 1 : 0).toInt();
    };
    qsizetype first = 0;
    while (first < tracks && isData(first))
        ++first;
    if (first == tracks)
        return -1;
    qsizetype last = first;
    while (last + 1 < tracks && !isData(last + 1))
        ++last;
    const int end = last + 1 < tracks ? start(last + 1) - Cdda::EnhancedCdSessionGap : start(tracks);
    return end - start(first);
}

}

int skipLast(const Cdda::Toc &toc)
{
    const qint64 samples = qint64(audioSectors(toc)) * Cdda::SamplesPerSector;
    return StrideSamples + int(samples % StrideSamples);
}

QString tocString(const Cdda::Toc &toc)
{
    QStringList parts;
    for (const Cdda::Track &t : toc.tracks)
        parts << (t.audio ? QString() : u"-"_s) + QString::number(t.firstLba);
    parts << QString::number(toc.leadOutLba);
    return parts.join(u':');
}

QString tocId(const Cdda::Toc &toc)
{
    const QList<int> audio = toc.audioTrackNumbers();
    if (audio.isEmpty())
        return {};
    const int start = toc.track(audio.first())->firstLba;
    const auto hex = [](int v) {
        return QByteArray::number(v, 16).toUpper().rightJustified(8, '0');
    };
    QByteArray text;
    for (qsizetype i = 1; i < audio.size(); ++i)
        text += hex(toc.track(audio.at(i))->firstLba - start);
    text += hex(audioSectors(toc));
    text += QByteArray(std::max<qsizetype>(0, (100 - audio.size()) * 8), '0');
    QByteArray id = QCryptographicHash::hash(text, QCryptographicHash::Sha1).toBase64();
    return QString::fromLatin1(id.replace('+', '.').replace('/', '_').replace('=', '-'));
}

QUrl lookupUrl(const Cdda::Toc &toc)
{
    QUrl url(u"%1/lookup2.php"_s.arg(QLatin1String(server)));
    QUrlQuery query;
    query.addQueryItem(u"version"_s, u"3"_s);
    query.addQueryItem(u"ctdb"_s, u"1"_s);
    query.addQueryItem(u"fuzzy"_s, u"1"_s); // also entries of other TOCs with the same audio
    query.addQueryItem(u"metadata"_s, u"none"_s);
    query.addQueryItem(u"toc"_s, tocString(toc));
    url.setQuery(query);
    return url;
}

QList<Entry> parseResponse(const QByteArray &xml, QString *error)
{
    QList<Entry> entries;
    bool root = false;
    QXmlStreamReader reader(xml);
    while (!reader.atEnd()) {
        if (reader.readNext() != QXmlStreamReader::StartElement)
            continue;
        if (reader.name() == u"ctdb") {
            root = true;
            continue;
        }
        if (reader.name() != u"entry")
            continue;
        const QXmlStreamAttributes a = reader.attributes();
        Entry e;
        bool ok = false;
        e.crc = a.value("crc32"_L1).toUInt(&ok, 16);
        e.toc = a.value("toc"_L1).toString();
        if (!ok || e.toc.isEmpty())
            continue;
        e.id = a.value("id"_L1).toLongLong();
        e.confidence = a.value("confidence"_L1).toInt();
        e.stride = a.value("stride"_L1).toInt();
        e.npar = a.value("npar"_L1).toInt();
        const QByteArray syndromes = QByteArray::fromBase64(a.value("syndrome"_L1).toLatin1());
        const QByteArray parity = QByteArray::fromBase64(a.value("parity"_L1).toLatin1());
        if (!syndromes.isEmpty()) {
            const qsizetype n = std::min<qsizetype>({syndromes.size() / 2, qsizetype(MaxParity), qsizetype(e.npar)});
            for (qsizetype i = 0; i < n; ++i)
                e.syndromes.append(qFromLittleEndian<quint16>(syndromes.constData() + 2 * i));
        } else if (parity.size() >= 16) { // old entries: 8 parity words
            QList<quint16> words;
            for (qsizetype i = 0; i < 8; ++i)
                words.append(qFromLittleEndian<quint16>(parity.constData() + 2 * i));
            e.syndromes = Parity::syndromesFromParity(words);
        }
        const QString parityUrl = a.value("hasparity"_L1).toString();
        if (!parityUrl.isEmpty() && !e.syndromes.isEmpty())
            e.parityUrl = parityUrl.startsWith(u'/') ? QLatin1String(server) + parityUrl : parityUrl;
        for (const QStringView crc : a.value("trackcrcs"_L1).split(u' ', Qt::SkipEmptyParts)) {
            e.trackCrcs.append(crc.toUInt(&ok, 16));
            if (!ok) {
                e.trackCrcs.clear();
                break;
            }
        }
        entries.append(e);
    }
    if (error && reader.hasError())
        *error = reader.errorString();
    else if (error && !root)
        *error = u"unexpected response"_s;
    return reader.hasError() || !root ? QList<Entry>() : entries;
}

QList<Entry> lookup(const Cdda::Toc &toc, QString *error, int timeoutMs, const std::function<bool()> &isCanceled)
{
    bool notFound = false;
    const QByteArray data = Online::fetchBlocking(lookupUrl(toc), error, &notFound, timeoutMs, isCanceled);
    if (data.isEmpty())
        return {};
    return parseResponse(data, error);
}

bool isSameAudio(const Entry &entry, const Cdda::Toc &toc)
{
    return entry.stride == StrideSamples && audioSectors(entry.toc) == audioSectors(toc);
}

QByteArray fetchParity(const Entry &entry, QString *error, int timeoutMs, const std::function<bool()> &isCanceled)
{
    const qint64 size = qint64(entry.syndromes.size()) * Stride * 2;
    const QByteArray range = "bytes=0-" + QByteArray::number(size - 1);
    QByteArray data = Online::fetchBlocking(QUrl(entry.parityUrl), error, nullptr, timeoutMs, isCanceled, {{"Range", range}});
    if (data.size() < size) {
        if (error && error->isEmpty())
            *error = u"incomplete recovery data (%1 of %2 bytes)"_s.arg(data.size()).arg(size);
        return {};
    }
    data.truncate(size);
    return data;
}

bool canConfirmTracks(const QList<Entry> &entries, const Cdda::Toc &toc)
{
    const qsizetype tracks = toc.audioTrackNumbers().size();
    return std::any_of(entries.cbegin(), entries.cend(), [&](const Entry &e) {
        return isSameAudio(e, toc) && e.trackCrcs.size() == tracks;
    });
}

int trackConfidence(const QList<Entry> &entries, const Rip::SegmentResult &track, const Cdda::Toc &toc)
{
    const QList<int> audio = toc.audioTrackNumbers();
    const qsizetype index = audio.indexOf(track.segment.trackNumber);
    if (index < 0 || !track.segment.accurateRip)
        return 0;

    QList<quint32> crcs{track.ctdbCrc};
    for (const Rip::ShiftedChecksums &c : track.accurateRipShifted)
        crcs.append(c.ctdb);

    int confidence = 0;
    for (const Entry &e : entries)
        if (isSameAudio(e, toc) && e.trackCrcs.size() == audio.size() && crcs.contains(e.trackCrcs.at(index)))
            confidence += e.confidence;
    return confidence;
}

QStringList formatVerification(const QList<Entry> &entries, const QList<Rip::SegmentResult> &segments, const Cdda::Toc &toc, int *mismatches)
{
    if (mismatches)
        *mismatches = 0;

    QStringList l;
    l << u"CUETools database (CTDB) verification"_s << QString();
    l << u"     TOC ID %1"_s.arg(tocId(toc));
    if (entries.isEmpty()) {
        l << u"     Disc not found in the CUETools database."_s;
        return l;
    }

    const QList<int> audio = toc.audioTrackNumbers();
    QList<const Rip::SegmentResult *> ripped(audio.size(), nullptr); // by audio index
    QList<int> shifts{0}; // other pressings found via AccurateRip
    for (const Rip::SegmentResult &s : segments) {
        const qsizetype index = audio.indexOf(s.segment.trackNumber);
        if (!s.segment.accurateRip || index < 0)
            continue;
        ripped[index] = &s;
        for (const Rip::ShiftedChecksums &c : s.accurateRipShifted)
            if (!shifts.contains(c.shift))
                shifts.append(c.shift);
    }
    const bool complete = !ripped.contains(nullptr);

    const auto windowBytes = [](const Rip::Segment &s) {
        return (qint64(s.checksumLast() - s.checksumFirst() + 1) * Cdda::SamplesPerSector - s.ctdbSkipFirst - s.ctdbSkipLast) * Cdda::BytesPerSample;
    };
    // CRCs at every shift (index shift + range), where the PCM around the windows is known
    const auto slide = [&](quint32 crc, qint64 length, const QByteArray &head, const QByteArray &tail) {
        return Rip::slidingCrc32(crc, length, head, tail, int(head.size() / (2 * Cdda::BytesPerSample)));
    };
    QList<QList<quint32>> sliding(ripped.size());
    for (qsizetype i = 0; i < ripped.size(); ++i)
        if (const Rip::SegmentResult *r = ripped.at(i); r && !r->ctdbHead.isEmpty())
            sliding[i] = slide(r->ctdbCrc, windowBytes(r->segment), r->ctdbHead, r->ctdbTail);

    const auto trackCrc = [&](qsizetype index, int shift) -> std::optional<quint32> {
        const Rip::SegmentResult *r = ripped.at(index);
        if (!r)
            return std::nullopt;
        if (shift == 0)
            return r->ctdbCrc;
        for (const Rip::ShiftedChecksums &c : r->accurateRipShifted)
            if (c.shift == shift)
                return c.ctdb;
        const qsizetype k = shift + sliding.at(index).size() / 2;
        if (k >= 0 && k < sliding.at(index).size())
            return sliding.at(index).at(k);
        return std::nullopt;
    };
    // the track windows are contiguous: the disc CRC is their concatenation
    const auto discCrc = [&](int shift) -> std::optional<quint32> {
        if (!complete)
            return std::nullopt;
        std::optional<quint32> crc;
        for (qsizetype i = 0; i < ripped.size(); ++i) {
            const std::optional<quint32> track = trackCrc(i, shift);
            if (!track)
                return std::nullopt;
            crc = crc ? Rip::crc32Combine(*crc, *track, windowBytes(ripped.at(i)->segment)) : *track;
        }
        return crc;
    };

    // other pressings without AccurateRip: the shift at which an entry
    // matches as a whole, or else the one most of its tracks agree on
    const bool searchable = std::any_of(sliding.cbegin(), sliding.cend(), [](const QList<quint32> &l) {
        return !l.isEmpty();
    });
    if (searchable) {
        QList<quint32> disc;
        if (complete && !ripped.first()->ctdbHead.isEmpty() && !ripped.last()->ctdbTail.isEmpty()) {
            qint64 length = 0;
            for (const Rip::SegmentResult *r : std::as_const(ripped))
                length += windowBytes(r->segment);
            if (const std::optional<quint32> crc = discCrc(0))
                disc = slide(*crc, length, ripped.first()->ctdbHead, ripped.last()->ctdbTail);
        }
        for (const Entry &e : entries) {
            if (!isSameAudio(e, toc))
                continue;
            if (const qsizetype k = disc.indexOf(e.crc); k >= 0) {
                if (const int shift = int(k - disc.size() / 2); !shifts.contains(shift))
                    shifts.append(shift);
                continue;
            }
            if (e.trackCrcs.size() != audio.size())
                continue;
            QMap<int, int> votes;
            for (qsizetype i = 0; i < ripped.size(); ++i)
                if (const qsizetype k = sliding.at(i).indexOf(e.trackCrcs.at(i)); k >= 0)
                    ++votes[int(k - sliding.at(i).size() / 2)];
            int best = 0;
            for (auto it = votes.cbegin(); it != votes.cend(); ++it)
                if (it.value() > votes.value(best, 0))
                    best = it.key();
            if (!votes.isEmpty() && !shifts.contains(best))
                shifts.append(best);
        }
    }
    const auto offsetText = [](int shift) {
        return shift > 0 ? u"+%1"_s.arg(shift) : QString::number(shift);
    };

    int total = 0;
    for (const Entry &e : entries)
        total += e.confidence;

    // whole disc
    QList<std::optional<int>> matched; // per entry: shift at which the disc CRC matches
    QList<bool> sameAudio; // per entry: same audio length and CRC windows as this disc
    bool comparable = false; // at least one entry could be compared as a whole
    for (const Entry &e : entries) {
        std::optional<int> shift;
        QString status;
        sameAudio.append(isSameAudio(e, toc));
        if (!sameAudio.last()) {
            status = u"different audio length or format"_s;
        } else if (!complete) {
            status = u"not compared, not all tracks were ripped"_s;
        } else {
            comparable = true;
            for (int s : std::as_const(shifts)) {
                if (discCrc(s) == e.crc) {
                    shift = s;
                    break;
                }
            }
            if (!shift)
                status = u"no match"_s;
            else if (*shift == 0)
                status = u"accurately ripped"_s;
            else
                status = u"accurately ripped, other pressing with offset %1"_s.arg(offsetText(*shift));
        }
        matched.append(shift);
        l << u"     [%1] (%2/%3) %4"_s.arg(hex8(e.crc)).arg(e.confidence).arg(total).arg(status);
    }
    const bool anyMatch = std::any_of(matched.cbegin(), matched.cend(), [](const std::optional<int> &m) {
        return m.has_value();
    });
    const qsizetype hintAt = l.size();
    l << QString();

    // single tracks: entries that match as a whole, or their track CRCs
    int accurate = 0;
    int contradicted = 0;
    int unknown = 0;
    for (qsizetype i = 0; i < ripped.size(); ++i) {
        if (!ripped.at(i))
            continue;
        int confidence = 0;
        bool checked = comparable;
        for (qsizetype k = 0; k < entries.size(); ++k) {
            const Entry &e = entries.at(k);
            if (matched.at(k)) {
                confidence += e.confidence;
                continue;
            }
            if (e.trackCrcs.size() != audio.size())
                continue;
            checked = checked || sameAudio.at(k);
            for (int s : std::as_const(shifts)) {
                if (trackCrc(i, s) == e.trackCrcs.at(i)) {
                    confidence += e.confidence;
                    break;
                }
            }
        }
        const QString track = u"Track %1"_s.arg(audio.at(i), 2, 10, QLatin1Char('0'));
        if (confidence > 0) {
            ++accurate;
            l << u"     %1: accurately ripped (confidence %2 of %3)"_s.arg(track).arg(confidence).arg(total);
        } else if (checked) {
            ++contradicted;
            l << u"     %1: NOT accurately ripped"_s.arg(track);
        } else {
            ++unknown;
            l << u"     %1: cannot be verified (no track checksums in the database)"_s.arg(track);
        }
    }

    if (comparable && !anyMatch && accurate == 0 && shifts.size() == 1 && !searchable)
        l.insert(hintAt, u"     Other pressings are only recognized when AccurateRip knows the disc."_s);
    if (mismatches)
        *mismatches = contradicted;
    l << QString();
    if (contradicted > 0)
        l << u"Warning: %1 track(s) do not match the CUETools database."_s.arg(contradicted);
    else if (accurate > 0 && unknown == 0)
        l << u"All tracks accurately ripped."_s;
    else
        l << u"%1 track(s) accurately ripped, %2 cannot be verified."_s.arg(accurate).arg(unknown);
    return l;
}

}
