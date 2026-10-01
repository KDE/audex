/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "subchannel.h"

#include "cdtext.h"
#include "sectorreader.h"

#include <QtEndian>

#include <algorithm>
#include <cstdlib>

using namespace Qt::StringLiterals;

namespace Audex::Cdda
{

namespace
{

int fromBcd(quint8 v, bool *ok)
{
    if ((v >> 4) > 9 || (v & 0x0F) > 9)
        *ok = false;
    return (v >> 4) * 10 + (v & 0x0F);
}

quint8 toBcd(int v)
{
    return quint8(((v / 10) << 4) | (v % 10));
}

// ISRC country and owner code: 0-9 -> 0x00-0x09, A-Z -> 0x11-0x2A
QChar isrcChar(int code)
{
    if (code <= 9)
        return QChar(u'0' + code);
    if (code >= 0x11 && code <= 0x2A)
        return QChar(u'A' + code - 0x11);
    return QChar();
}

int isrcCode(QChar c)
{
    return c.isDigit() ? c.digitValue() : 0x11 + (c.toUpper().unicode() - u'A');
}

// 13 (MCN) or 7 (ISRC) BCD digits packed from the high nibble of `first`
QString digits(QByteArrayView f, int first, int count, bool *ok)
{
    QString s;
    for (int i = 0; i < count; ++i) {
        const quint8 byte = quint8(f.at(first + i / 2));
        const int d = i % 2 == 0 ? byte >> 4 : byte & 0x0F;
        if (d > 9)
            *ok = false;
        s += QChar(u'0' + d);
    }
    return s;
}

void putDigits(QByteArray &f, int first, const QString &s)
{
    for (int i = 0; i < s.size(); ++i) {
        const int d = s.at(i).digitValue();
        f[first + i / 2] = char(quint8(f.at(first + i / 2)) | (i % 2 == 0 ? d << 4 : d));
    }
}

}

std::optional<SubQ> parseSubQ(QByteArrayView f)
{
    if (f.size() < SubQBytes)
        return std::nullopt;
    const quint16 stored = qFromBigEndian<quint16>(f.constData() + 10);
    if (stored != 0 && quint16(~cdTextCrc(f.first(10))) != stored) // same CRC-16 as CD-Text
        return std::nullopt;

    SubQ q;
    q.control = quint8(f.at(0)) >> 4;
    q.adr = quint8(f.at(0)) & 0x0F;
    bool ok = true;
    switch (q.adr) {
    case 1: {
        const quint8 tno = quint8(f.at(1));
        q.track = tno == 0xAA ? 0xAA : fromBcd(tno, &ok); // 0xAA: lead-out
        q.index = fromBcd(quint8(f.at(2)), &ok);
        q.relative = (fromBcd(quint8(f.at(3)), &ok) * 60 + fromBcd(quint8(f.at(4)), &ok)) * SectorsPerSecond + fromBcd(quint8(f.at(5)), &ok);
        q.absoluteLba = msfToLba(fromBcd(quint8(f.at(7)), &ok), fromBcd(quint8(f.at(8)), &ok), fromBcd(quint8(f.at(9)), &ok));
        break;
    }
    case 2:
        q.mcn = digits(f, 1, 13, &ok);
        break;
    case 3: {
        const quint32 bits = qFromBigEndian<quint32>(f.constData() + 1); // five 6 bit characters
        for (int i = 0; i < 5; ++i) {
            const QChar c = isrcChar(int(bits >> (26 - 6 * i)) & 0x3F);
            if (c.isNull())
                ok = false;
            q.isrc += c;
        }
        q.isrc += digits(f, 5, 7, &ok);
        break;
    }
    default:
        return std::nullopt;
    }
    if (!ok)
        return std::nullopt;
    return q;
}

QByteArray formatSubQ(const SubQ &q)
{
    QByteArray f(SubQBytes, '\0');
    f[0] = char((q.control << 4) | (q.adr & 0x0F));
    if (q.adr == 1) {
        f[1] = char(q.track == 0xAA ? 0xAA : toBcd(q.track));
        f[2] = char(toBcd(q.index));
        f[3] = char(toBcd(q.relative / SectorsPerSecond / 60));
        f[4] = char(toBcd(q.relative / SectorsPerSecond % 60));
        f[5] = char(toBcd(q.relative % SectorsPerSecond));
        const int absolute = q.absoluteLba + PregapSectors;
        f[7] = char(toBcd(absolute / SectorsPerSecond / 60));
        f[8] = char(toBcd(absolute / SectorsPerSecond % 60));
        f[9] = char(toBcd(absolute % SectorsPerSecond));
    } else if (q.adr == 2) {
        putDigits(f, 1, q.mcn);
    } else if (q.adr == 3) {
        quint32 bits = 0;
        for (int i = 0; i < 5; ++i)
            bits |= quint32(isrcCode(q.isrc.at(i))) << (26 - 6 * i);
        qToBigEndian<quint32>(bits, f.data() + 1);
        putDigits(f, 5, q.isrc.mid(5, 7));
    }
    qToBigEndian<quint16>(quint16(~cdTextCrc(f.first(10))), f.data() + 10);
    return f;
}

SubchannelScan scanSubchannel(Rip::SectorReader &reader, const Toc &toc, bool isrcMcn, const std::function<bool()> &isCanceled)
{
    SubchannelScan scan;
    // A drive that does not answer a Q read in time would make the scan wait
    // for the timeout on each of its many reads: it ends at the first one.
    const int timeoutsBefore = reader.commandTimeouts();
    const auto timedOut = [&] {
        return reader.commandTimeouts() > timeoutsBefore;
    };
    const auto canceled = [&] {
        return (isCanceled && isCanceled()) || timedOut();
    };

    // Q frames of [lba, lba + count), clipped to the audio area; frames with
    // a broken CRC are left out.
    struct Frame {
        int requested = 0; // sector the frame was delivered with
        SubQ q;
    };
    const auto readFrames = [&](int lba, int count) {
        QList<Frame> frames;
        const int first = std::max(lba, toc.audioStartLba());
        const int end = std::min(lba + count, toc.audioEndLba());
        if (first >= end || timedOut())
            return frames;
        const QByteArray raw = reader.readSubchannelQ(first, end - first);
        for (qsizetype i = 0; (i + 1) * SubQBytes <= raw.size(); ++i)
            if (const auto q = parseSubQ(QByteArrayView(raw).sliced(i * SubQBytes, SubQBytes)))
                frames.append(Frame{first + int(i), *q});
        return frames;
    };

    // Many drives deliver the Q frame of a neighbouring sector: sector n
    // comes with the frame of n + shift. The shift is measured once; frames
    // are identified by their absolute address, not by the sector they came with.
    int shift = 0;
    constexpr int Window = 2;

    // Position frame of a sector, read again if its CRC fails. MCN/ISRC frames
    // carry no position; then a neighbour stands in (at most one sector off
    // at an index boundary).
    const auto positionAt = [&](int lba) -> std::optional<SubQ> {
        std::optional<SubQ> neighbour;
        for (int attempt = 0; attempt < 4; ++attempt) {
            const QList<Frame> frames = readFrames(lba - shift - Window, 2 * Window + 1);
            for (const Frame &f : frames) {
                if (f.q.adr != 1)
                    continue;
                if (f.q.absoluteLba == lba)
                    return f.q;
                if (!neighbour && std::abs(f.q.absoluteLba - lba) == 1)
                    neighbour = f.q;
            }
            // every frame arrived intact: lba is an MCN/ISRC frame, reading again does not help
            if (neighbour && frames.size() == 2 * Window + 1)
                break;
        }
        return neighbour;
    };
    // first sector in [lo, hi] that satisfies pred (monotonic, pred(hi) holds); -1 on failure
    const auto firstWhere = [&](int lo, int hi, const std::function<bool(const SubQ &)> &pred) {
        while (lo < hi) {
            if (canceled())
                return -1;
            const int mid = lo + (hi - lo) / 2;
            const auto q = positionAt(mid);
            if (!q)
                return -1;
            if (pred(*q))
                hi = mid;
            else
                lo = mid + 1;
        }
        return lo;
    };
    // most frequent MCN (adr 2) or ISRC (adr 3) in [lba, lba + count)
    const auto collect = [&](int lba, int count, quint8 adr) {
        QMap<QString, int> votes;
        for (int done = 0; done < count && !canceled();) {
            const int n = std::min(std::max(1, reader.maxSectorsPerRead()), count - done);
            const QByteArray raw = reader.readSubchannelQ(lba + done, n);
            const int delivered = int(raw.size() / SubQBytes);
            if (delivered == 0)
                break;
            for (qsizetype i = 0; i < delivered; ++i) {
                const auto q = parseSubQ(QByteArrayView(raw).sliced(i * SubQBytes, SubQBytes));
                if (q && q->adr == adr) {
                    const QString value = adr == 2 ? q->mcn : q->isrc;
                    if (value.count(u'0') != value.size())
                        ++votes[value];
                }
            }
            // a short read stopped at a sector the drive could not read:
            // continue behind it instead of skipping the rest of the command
            done += delivered < n ? delivered + 1 : n;
        }
        QString best;
        for (auto it = votes.cbegin(); it != votes.cend(); ++it)
            if (best.isEmpty() || it.value() > votes.value(best))
                best = it.key();
        return best;
    };

    const QList<int> audio = toc.audioTrackNumbers();
    if (audio.isEmpty())
        return scan;
    QMap<int, int> shifts; // shift -> frames
    for (int number : {audio.first(), audio.at(audio.size() / 2), audio.last()}) {
        const Track *t = toc.track(number);
        for (const Frame &f : readFrames(t->firstLba + std::min(t->sectorCount() / 2, 150), 16))
            if (f.q.adr == 1)
                ++shifts[f.q.absoluteLba - f.requested];
    }
    if (shifts.isEmpty()) {
        scan.notes << (timedOut() ? u"The drive did not answer a Q sub-channel read in time."_s : u"The drive delivers no usable Q sub-channel data."_s);
        return scan;
    }
    for (auto it = shifts.cbegin(); it != shifts.cend(); ++it)
        if (it.value() > shifts.value(shift))
            shift = it.key();
    if (shift != 0)
        scan.notes << u"The drive delivers the Q sub-channel %1 sector(s) off; corrected."_s.arg(shift);
    scan.supported = true;

    // pregaps: the sectors before index 01 of track n that already belong to track n
    for (int i = 1; i < audio.size() && !canceled(); ++i) {
        const Track *t = toc.track(audio.at(i));
        const Track *prev = toc.track(audio.at(i - 1));
        const int n = t->number;
        if (prev->number != n - 1)
            continue; // a data track lies in between
        const auto belongsToTrack = [n](const SubQ &q) {
            return q.track >= n;
        };
        const auto last = positionAt(t->firstLba - 1);
        if (!last) {
            scan.notes << u"Track %1: no Q data before index 01."_s.arg(n);
            continue;
        }
        if (last->track != n)
            continue; // no pregap
        int hi = t->firstLba - 1;
        int lo = hi;
        for (int step = 64; lo > prev->firstLba; step *= 2) { // search backwards for the previous track
            lo = std::max(prev->firstLba, t->firstLba - step);
            const auto q = positionAt(lo);
            if (!q)
                continue;
            if (!belongsToTrack(*q))
                break;
            hi = lo;
        }
        const int start = firstWhere(lo, hi, belongsToTrack);
        if (start > prev->firstLba)
            scan.index00.insert(n, start);
    }

    // index 02 and later
    for (int i = 0; i < audio.size() && !canceled(); ++i) {
        const Track *t = toc.track(audio.at(i));
        const int n = t->number;
        const int next = i + 1 < audio.size() ? audio.at(i + 1) : 0;
        const int end = scan.index00.contains(next) ? scan.index00.value(next) - 1 : t->lastLba;
        // a few sectors before the end: at the end of the disc the frames of
        // the last sectors are out of reach of a drive that delivers them late
        const auto q = positionAt(std::max(t->firstLba, end - 2 * Window));
        if (!q || q->track != n || q->index <= 1)
            continue;
        for (int k = 2; k <= q->index; ++k) {
            const int at = firstWhere(t->firstLba, end, [n, k](const SubQ &f) {
                return f.track > n || (f.track == n && f.index >= k);
            });
            if (at < 0)
                break;
            scan.indexes[n].append(at);
        }
    }

    if (isrcMcn) {
        // ISRC and media catalog number: each appears at least once per 100 frames
        for (int number : audio) {
            if (canceled())
                break;
            const Track *t = toc.track(number);
            const QString isrc = collect(t->firstLba, std::min(t->sectorCount(), 200), 3);
            if (!isrc.isEmpty())
                scan.isrc.insert(number, isrc);
        }
        const Track *first = toc.track(audio.first());
        scan.mcn = collect(first->firstLba, std::min(first->sectorCount(), 200), 2);
    }

    if (timedOut())
        scan.notes << u"The drive did not answer a Q sub-channel read in time; the scan was stopped there."_s;

    return scan;
}

QStringList formatSubchannel(const SubchannelScan &scan, const Toc &toc)
{
    QStringList l;
    l << u"Pregaps, indexes and ISRC (Q sub-channel)"_s << QString();
    if (!scan.supported) {
        l << u"     Not available."_s;
        for (const QString &note : scan.notes)
            l << u"     %1"_s.arg(note);
        return l;
    }
    const qsizetype header = l.size();
    for (const Track &t : toc.tracks) {
        if (!t.audio)
            continue;
        QStringList parts;
        if (scan.index00.contains(t.number))
            parts << u"pregap %1"_s.arg(sectorsToMsf(t.firstLba - scan.index00.value(t.number)));
        const QList<int> indexes = scan.indexes.value(t.number);
        for (int k = 0; k < indexes.size(); ++k)
            parts << u"index %1 at %2"_s.arg(k + 2, 2, 10, QLatin1Char('0')).arg(sectorsToMsf(indexes.at(k) - t.firstLba));
        if (scan.isrc.contains(t.number))
            parts << u"ISRC %1"_s.arg(scan.isrc.value(t.number));
        if (!parts.isEmpty())
            l << u"     Track %1: %2"_s.arg(t.number, 2, 10, QLatin1Char('0')).arg(parts.join(u", "_s));
    }
    if (!scan.mcn.isEmpty())
        l << u"     Media catalog number %1"_s.arg(scan.mcn);
    if (l.size() == header)
        l << u"     No pregaps, further indexes or ISRCs."_s;
    for (const QString &note : scan.notes)
        l << u"     %1"_s.arg(note);
    return l;
}

}
