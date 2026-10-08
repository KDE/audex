/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// Pure parsers for MMC/SPC response data. No I/O, no platform dependencies,
// so everything in here can be unit tested with captured hex dumps.
//
// All wire formats are decoded with explicit byte offsets and bit masks.
// No bitfield structs: their layout is implementation defined.

#include <QByteArrayView>
#include <QList>
#include <QString>

#include <optional>

#include "toc.h"

namespace Audex::Mmc
{

// ---- Sense data (SPC-4 fixed and descriptor format) ------------------------

struct Sense {
    bool valid = false;
    quint8 responseCode = 0;
    quint8 key = 0;
    quint8 asc = 0;
    quint8 ascq = 0;

    bool informationValid = false;
    qint64 information = 0; // e.g. the LBA of a recovered or failed block

    QString toString() const;
};

Sense parseSense(QByteArrayView data);
QString senseKeyName(quint8 key);
QString ascDescription(quint8 asc, quint8 ascq); // empty if unknown

// ---- INQUIRY ---------------------------------------------------------------

struct InquiryData {
    quint8 deviceType = 0; // 0x05 = CD/DVD device
    QString vendor;
    QString model;
    QString revision;
};

std::optional<InquiryData> parseInquiry(QByteArrayView data);

// ---- MODE SENSE(10), page 2Ah (CD/DVD capabilities) -----------------------

struct Capabilities {
    bool audioPlay = false;
    bool cddaSupported = false; // READ CD for audio sectors
    bool accurateStream = false; // "CD-DA stream is accurate"
    bool rwSupported = false;
    bool rwDeinterleaved = false;
    bool c2Pointers = false;
    bool cdrWrite = false; // a writer: it reads the ATIP of every CD-R
    bool isrc = false;
    bool upc = false;
    bool eject = false;
    bool lock = false;
    int loadingMechanism = 0;
    int maxReadSpeedKbps = 0; // obsolete since MMC-4, often still reported
    int currentReadSpeedKbps = 0; // obsolete since MMC-4
    int bufferSizeKb = 0;
};

// Expects the complete MODE SENSE(10) response including the 8 byte header.
std::optional<Capabilities> parseCapabilitiesPage(QByteArrayView modeSense10);

// ---- the medium: ATIP and current profile ----------------------------------

// ATIP of a CD-R/RW (READ TOC/PMA/ATIP format 0100b), from the pregroove.
// Pressed discs have none.
struct Atip {
    bool rewritable = false; // CD-RW
    int leadInMinute = 0; // start of the lead-in: identifies the manufacturer
    int leadInSecond = 0;
    int leadInFrame = 0;
    int leadOutMinute = 0; // last possible start of the lead-out: the capacity
    int leadOutSecond = 0;
    int leadOutFrame = 0;
};

// Expects the complete response including the 4 byte header; nothing if it
// carries no ATIP descriptor
std::optional<Atip> parseAtip(QByteArrayView response);

// Current profile from the header of GET CONFIGURATION (0x0008 CD-ROM,
// 0x0009 CD-R, 0x000A CD-RW)
std::optional<quint16> parseCurrentProfile(QByteArrayView response);

// ---- READ TOC/PMA/ATIP ------------------------------------------------------

// Format 0010b (raw / full TOC). One entry is 11 bytes.
struct FullTocEntry {
    quint8 session = 0;
    quint8 adr = 0;
    quint8 control = 0;
    quint8 tno = 0;
    quint8 point = 0;
    quint8 min = 0, sec = 0, frame = 0;
    quint8 zero = 0;
    quint8 pmin = 0, psec = 0, pframe = 0;
};

std::optional<QList<FullTocEntry>> parseFullToc(QByteArrayView data);
std::optional<Cdda::Toc> tocFromFullToc(const QList<FullTocEntry> &entries, QString *error = nullptr);

// Format 0000b (formatted TOC), requested with MSF bit cleared (LBA addresses).
struct FormattedTocEntry {
    quint8 adr = 0;
    quint8 control = 0;
    quint8 track = 0; // 0xAA == lead-out
    qint32 lba = 0;
};

std::optional<QList<FormattedTocEntry>> parseFormattedToc(QByteArrayView data);

// The formatted TOC has no session information. An Enhanced CD (audio tracks
// followed by a data track) is detected heuristically and the last audio track
// is shortened by the session gap (11400 sectors).
std::optional<Cdda::Toc> tocFromFormattedToc(const QList<FormattedTocEntry> &entries, QString *error = nullptr);

// Control nibble helpers (Q sub-channel CONTROL field)
inline constexpr bool controlIsData(quint8 control)
{
    return control & 0x04;
}
inline constexpr bool controlPreEmphasis(quint8 control)
{
    return (control & 0x05) == 0x01; // audio track with pre-emphasis
}
inline constexpr bool controlCopyPermitted(quint8 control)
{
    return control & 0x02;
}
inline constexpr bool controlFourChannel(quint8 control)
{
    return (control & 0x0C) == 0x08;
}

}
