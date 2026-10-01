/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mmcparse.h"

#include <QMap>
#include <QtEndian>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Audex::Mmc
{

namespace
{

inline quint8 u8(QByteArrayView d, qsizetype i)
{
    return static_cast<quint8>(d.at(i));
}

inline quint16 be16(QByteArrayView d, qsizetype i)
{
    return qFromBigEndian<quint16>(d.constData() + i);
}

inline qint32 be32s(QByteArrayView d, qsizetype i)
{
    return qFromBigEndian<qint32>(d.constData() + i);
}

QString trimmedAscii(QByteArrayView d, qsizetype pos, qsizetype len)
{
    return QString::fromLatin1(d.sliced(pos, len)).simplified();
}

bool setError(QString *error, const QString &msg)
{
    if (error)
        *error = msg;
    return false;
}

}

// ---------------------------------------------------------------------------

QString senseKeyName(quint8 key)
{
    switch (key & 0x0F) {
    case 0x0:
        return u"No Sense"_s;
    case 0x1:
        return u"Recovered Error"_s;
    case 0x2:
        return u"Not Ready"_s;
    case 0x3:
        return u"Medium Error"_s;
    case 0x4:
        return u"Hardware Error"_s;
    case 0x5:
        return u"Illegal Request"_s;
    case 0x6:
        return u"Unit Attention"_s;
    case 0x7:
        return u"Data Protect"_s;
    case 0x8:
        return u"Blank Check"_s;
    case 0x9:
        return u"Vendor Specific"_s;
    case 0xA:
        return u"Copy Aborted"_s;
    case 0xB:
        return u"Aborted Command"_s;
    case 0xD:
        return u"Volume Overflow"_s;
    case 0xE:
        return u"Miscompare"_s;
    }
    return u"Reserved"_s;
}

QString ascDescription(quint8 asc, quint8 ascq)
{
    switch (asc) {
    case 0x00:
        return ascq == 0 ? u"No additional sense information"_s : QString();
    case 0x02:
        return u"No seek complete"_s;
    case 0x04:
        switch (ascq) {
        case 0x00:
            return u"Logical unit not ready, cause not reportable"_s;
        case 0x01:
            return u"Logical unit is in process of becoming ready"_s;
        case 0x02:
            return u"Logical unit not ready, initializing command required"_s;
        default:
            return u"Logical unit not ready"_s;
        }
    case 0x11:
        switch (ascq) {
        case 0x00:
            return u"Unrecovered read error"_s;
        case 0x05:
            return u"L-EC uncorrectable error"_s;
        case 0x06:
            return u"CIRC unrecovered error"_s;
        default:
            return u"Unrecovered read error"_s;
        }
    case 0x15:
        return u"Positioning error"_s;
    case 0x17:
        return u"Recovered data with no error correction applied"_s;
    case 0x18:
        return u"Recovered data with error correction applied"_s;
    case 0x20:
        return u"Invalid command operation code"_s;
    case 0x21:
        return u"Logical block address out of range"_s;
    case 0x24:
        return u"Invalid field in CDB"_s;
    case 0x28:
        return u"Not ready to ready change, medium may have changed"_s;
    case 0x29:
        return u"Power on, reset or bus device reset occurred"_s;
    case 0x30:
        return u"Incompatible medium installed"_s;
    case 0x3A:
        return u"Medium not present"_s;
    case 0x44:
        return u"Internal target failure"_s;
    case 0x57:
        return u"Unable to recover table of contents"_s;
    case 0x63:
        return u"End of user area encountered on this track"_s;
    case 0x64:
        return u"Illegal mode for this track"_s;
    case 0x6F:
        return u"Copy protection error"_s;
    }
    return QString();
}

QString Sense::toString() const
{
    if (!valid)
        return u"no sense data"_s;
    QString s =
        u"sense key %1h (%2), ASC/ASCQ %3h/%4h"_s.arg(key, 0, 16).arg(senseKeyName(key)).arg(asc, 2, 16, QLatin1Char('0')).arg(ascq, 2, 16, QLatin1Char('0'));
    const QString d = ascDescription(asc, ascq);
    if (!d.isEmpty())
        s += u" (%1)"_s.arg(d);
    return s;
}

Sense parseSense(QByteArrayView d)
{
    Sense s;
    if (d.size() < 1)
        return s;
    s.responseCode = u8(d, 0) & 0x7F;
    if (s.responseCode == 0x70 || s.responseCode == 0x71) {
        if (d.size() < 3)
            return s;
        s.key = u8(d, 2) & 0x0F;
        s.asc = d.size() > 12 ? u8(d, 12) : 0;
        s.ascq = d.size() > 13 ? u8(d, 13) : 0;
        s.valid = true;
        if ((u8(d, 0) & 0x80) && d.size() >= 7) {
            s.informationValid = true;
            s.information = be32s(d, 3);
        }
    } else if (s.responseCode == 0x72 || s.responseCode == 0x73) {
        if (d.size() < 4)
            return s;
        s.key = u8(d, 1) & 0x0F;
        s.asc = u8(d, 2);
        s.ascq = u8(d, 3);
        s.valid = true;
        // descriptor format: look for the information descriptor (type 00h)
        for (qsizetype pos = 8; pos + 2 <= d.size(); pos += 2 + u8(d, pos + 1)) {
            if (u8(d, pos) == 0x00 && pos + 12 <= d.size() && (u8(d, pos + 2) & 0x80)) {
                s.informationValid = true;
                s.information = qFromBigEndian<qint64>(d.constData() + pos + 4);
                break;
            }
        }
    }
    return s;
}

// ---------------------------------------------------------------------------

std::optional<InquiryData> parseInquiry(QByteArrayView d)
{
    if (d.size() < 36)
        return std::nullopt;
    InquiryData r;
    r.deviceType = u8(d, 0) & 0x1F;
    r.vendor = trimmedAscii(d, 8, 8);
    r.model = trimmedAscii(d, 16, 16);
    r.revision = trimmedAscii(d, 32, 4);
    return r;
}

// ---------------------------------------------------------------------------

std::optional<Capabilities> parseCapabilitiesPage(QByteArrayView d)
{
    if (d.size() < 8)
        return std::nullopt;

    // Some drives ignore the DBD bit, so honour the block descriptor length.
    const qsizetype pageOffset = 8 + be16(d, 6);
    if (d.size() < pageOffset + 16)
        return std::nullopt;

    const QByteArrayView p = d.sliced(pageOffset);
    if ((u8(p, 0) & 0x3F) != 0x2A)
        return std::nullopt;
    if (u8(p, 1) < 14) // we need bytes 2..15
        return std::nullopt;

    Capabilities c;
    const quint8 b3 = u8(p, 3);
    const quint8 b4 = u8(p, 4);
    const quint8 b5 = u8(p, 5);
    const quint8 b6 = u8(p, 6);

    c.cdrWrite = b3 & 0x01;
    c.audioPlay = b4 & 0x01;

    c.cddaSupported = b5 & 0x01;
    c.accurateStream = b5 & 0x02;
    c.rwSupported = b5 & 0x04;
    c.rwDeinterleaved = b5 & 0x08;
    c.c2Pointers = b5 & 0x10;
    c.isrc = b5 & 0x20;
    c.upc = b5 & 0x40;

    c.lock = b6 & 0x01;
    c.eject = b6 & 0x08;
    c.loadingMechanism = (b6 >> 5) & 0x07;

    c.maxReadSpeedKbps = be16(p, 8);
    c.bufferSizeKb = be16(p, 12);
    c.currentReadSpeedKbps = be16(p, 14);
    return c;
}

// ---------------------------------------------------------------------------

std::optional<QList<FullTocEntry>> parseFullToc(QByteArrayView d)
{
    if (d.size() < 4)
        return std::nullopt;
    // Data length excludes the length field itself.
    const qsizetype len = std::min<qsizetype>(d.size(), qsizetype(be16(d, 0)) + 2);
    if (len < 4)
        return std::nullopt;

    QList<FullTocEntry> entries;
    for (qsizetype pos = 4; pos + 11 <= len; pos += 11) {
        FullTocEntry e;
        e.session = u8(d, pos);
        e.adr = u8(d, pos + 1) >> 4;
        e.control = u8(d, pos + 1) & 0x0F;
        e.tno = u8(d, pos + 2);
        e.point = u8(d, pos + 3);
        e.min = u8(d, pos + 4);
        e.sec = u8(d, pos + 5);
        e.frame = u8(d, pos + 6);
        e.zero = u8(d, pos + 7);
        e.pmin = u8(d, pos + 8);
        e.psec = u8(d, pos + 9);
        e.pframe = u8(d, pos + 10);
        entries.append(e);
    }
    return entries;
}

std::optional<Cdda::Toc> tocFromFullToc(const QList<FullTocEntry> &entries, QString *error)
{
    struct SessionInfo {
        int firstTrack = -1;
        int lastTrack = -1;
        int leadOut = -1;
    };

    QMap<int, SessionInfo> sessions;
    QMap<int, Cdda::Track> tracks;

    // Collect first, compute afterwards: MMC does not specify the order of the
    // descriptors, drives differ (A0/A1/A2 before or after the track points).
    for (const FullTocEntry &e : entries) {
        if (e.adr != 1)
            continue; // ADR 5 (multi-session pointers, B0/C0) etc. are not needed here

        if (e.point >= 1 && e.point <= 99) {
            Cdda::Track t;
            t.number = e.point;
            t.session = e.session;
            t.firstLba = Cdda::msfToLba(e.pmin, e.psec, e.pframe);
            t.audio = !controlIsData(e.control);
            t.preEmphasis = controlPreEmphasis(e.control);
            t.copyPermitted = controlCopyPermitted(e.control);
            t.fourChannel = controlFourChannel(e.control);
            tracks.insert(t.number, t);
        } else if (e.point == 0xA0) {
            sessions[e.session].firstTrack = e.pmin;
        } else if (e.point == 0xA1) {
            sessions[e.session].lastTrack = e.pmin;
        } else if (e.point == 0xA2) {
            sessions[e.session].leadOut = Cdda::msfToLba(e.pmin, e.psec, e.pframe);
        }
    }

    if (tracks.isEmpty()) {
        setError(error, u"Full TOC contains no track descriptors"_s);
        return std::nullopt;
    }

    Cdda::Toc toc;
    toc.source = u"full TOC"_s;
    toc.tracks = tracks.values(); // QMap keeps keys sorted

    for (int i = 0; i < toc.tracks.size(); ++i) {
        Cdda::Track &t = toc.tracks[i];
        const bool hasNextInSession = i + 1 < toc.tracks.size() && toc.tracks.at(i + 1).session == t.session;
        if (hasNextInSession) {
            t.lastLba = toc.tracks.at(i + 1).firstLba - 1;
        } else {
            const SessionInfo si = sessions.value(t.session);
            if (si.leadOut < 0) {
                setError(error, u"Full TOC has no lead-out (A2) for session %1"_s.arg(t.session));
                return std::nullopt;
            }
            t.lastLba = si.leadOut - 1;
        }
    }

    const SessionInfo lastSession = sessions.value(toc.tracks.last().session);
    toc.leadOutLba = lastSession.leadOut;

    QString validationError;
    if (!toc.isValid(&validationError)) {
        setError(error, validationError);
        return std::nullopt;
    }
    return toc;
}

// ---------------------------------------------------------------------------

std::optional<QList<FormattedTocEntry>> parseFormattedToc(QByteArrayView d)
{
    if (d.size() < 4)
        return std::nullopt;
    const qsizetype len = std::min<qsizetype>(d.size(), qsizetype(be16(d, 0)) + 2);
    if (len < 4)
        return std::nullopt;

    QList<FormattedTocEntry> entries;
    for (qsizetype pos = 4; pos + 8 <= len; pos += 8) {
        FormattedTocEntry e;
        e.adr = u8(d, pos + 1) >> 4;
        e.control = u8(d, pos + 1) & 0x0F;
        e.track = u8(d, pos + 2);
        e.lba = be32s(d, pos + 4);
        entries.append(e);
    }
    return entries;
}

std::optional<Cdda::Toc> tocFromFormattedToc(const QList<FormattedTocEntry> &entries, QString *error)
{
    Cdda::Toc toc;
    toc.source = u"formatted TOC"_s;
    int leadOut = -1;

    for (const FormattedTocEntry &e : entries) {
        if (e.track == 0xAA) {
            leadOut = e.lba;
        } else if (e.track >= 1 && e.track <= 99) {
            Cdda::Track t;
            t.number = e.track;
            t.firstLba = e.lba;
            t.audio = !controlIsData(e.control);
            t.preEmphasis = controlPreEmphasis(e.control);
            t.copyPermitted = controlCopyPermitted(e.control);
            t.fourChannel = controlFourChannel(e.control);
            toc.tracks.append(t);
        }
    }

    if (toc.tracks.isEmpty() || leadOut < 0) {
        setError(error, u"Formatted TOC is incomplete"_s);
        return std::nullopt;
    }

    std::sort(toc.tracks.begin(), toc.tracks.end(), [](const Cdda::Track &a, const Cdda::Track &b) {
        return a.number < b.number;
    });

    for (int i = 0; i < toc.tracks.size(); ++i) {
        Cdda::Track &t = toc.tracks[i];
        if (i + 1 < toc.tracks.size()) {
            const Cdda::Track &next = toc.tracks.at(i + 1);
            t.lastLba = next.firstLba - 1;
            const bool lastIsData = i + 2 == toc.tracks.size() && !next.audio;
            if (t.audio && lastIsData && next.firstLba - Cdda::EnhancedCdSessionGap > t.firstLba) {
                // Enhanced CD heuristic
                t.lastLba = next.firstLba - Cdda::EnhancedCdSessionGap - 1;
                toc.tracks[i + 1].session = 2;
                toc.source = u"formatted TOC (Enhanced CD layout assumed)"_s;
            }
        } else {
            t.lastLba = leadOut - 1;
        }
    }
    toc.leadOutLba = leadOut;

    QString validationError;
    if (!toc.isValid(&validationError)) {
        setError(error, validationError);
        return std::nullopt;
    }
    return toc;
}

// ---- the medium ------------------------------------------------------------

std::optional<Atip> parseAtip(QByteArrayView d)
{
    constexpr int Header = 4;
    if (d.size() < Header + 11 || qFromBigEndian<quint16>(d.data()) < 2 + 11)
        return std::nullopt;
    const auto byte = [&](int i) {
        return int(static_cast<unsigned char>(d[Header + i]));
    };
    Atip a;
    a.rewritable = byte(2) & 0x40; // disc type
    a.leadInMinute = byte(4);
    a.leadInSecond = byte(5);
    a.leadInFrame = byte(6);
    a.leadOutMinute = byte(8);
    a.leadOutSecond = byte(9);
    a.leadOutFrame = byte(10);
    const auto valid = [](int m, int s, int f) {
        return m < 100 && s < 60 && f < 75 && (m | s | f) != 0;
    };
    if (!valid(a.leadInMinute, a.leadInSecond, a.leadInFrame) || !valid(a.leadOutMinute, a.leadOutSecond, a.leadOutFrame))
        return std::nullopt;
    return a;
}

std::optional<quint16> parseCurrentProfile(QByteArrayView d)
{
    // data length: the bytes after itself, the header needs 4 of them
    if (d.size() < 8 || qFromBigEndian<quint32>(d.data()) < 4)
        return std::nullopt;
    return qFromBigEndian<quint16>(d.data() + 6);
}

}
