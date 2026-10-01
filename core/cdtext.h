/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// CD-Text decoder for the READ TOC/PMA/ATIP format 0101b response.
//
// A CD-Text area consists of up to 8 blocks (languages). Each block is a
// sequence of 18 byte packs: 4 header bytes, 12 payload bytes, 2 CRC bytes.
// Strings are NUL separated (double NUL for double byte character sets) and
// may span packs; a TAB means "same as the previous track".

#include <QByteArrayView>
#include <QMap>
#include <QString>
#include <QStringList>

#include <optional>

namespace Audex::Cdda
{

namespace CdTextPack
{
inline constexpr quint8 Title = 0x80;
inline constexpr quint8 Performer = 0x81;
inline constexpr quint8 Songwriter = 0x82;
inline constexpr quint8 Composer = 0x83;
inline constexpr quint8 Arranger = 0x84;
inline constexpr quint8 Message = 0x85;
inline constexpr quint8 DiscId = 0x86;
inline constexpr quint8 Genre = 0x87;
inline constexpr quint8 TocInfo = 0x88;
inline constexpr quint8 TocInfo2 = 0x89;
inline constexpr quint8 ClosedInfo = 0x8D;
inline constexpr quint8 UpcIsrc = 0x8E;
inline constexpr quint8 SizeInfo = 0x8F;
}

namespace CdTextCharset
{
inline constexpr quint8 Iso8859_1 = 0x00;
inline constexpr quint8 Ascii = 0x01;
inline constexpr quint8 MsJis = 0x80;
inline constexpr quint8 Korean = 0x81;
inline constexpr quint8 Mandarin = 0x82;
}

struct CdTextBlock {
    int block = 0; // 0..7
    quint8 characterCode = CdTextCharset::Iso8859_1;
    quint8 languageCode = 0; // EBU Tech 3258, 0x09 = English, 0x08 = German
    int firstTrack = 0; // from the size info pack, 0 if missing
    int lastTrack = 0;
    bool hasSizeInfo = false;

    QMap<quint8, QMap<int, QString>> texts; // pack type -> track (0 = disc) -> text
    int genreCode = 0;
    QString genreText;

    QString text(quint8 packType, int track) const
    {
        return texts.value(packType).value(track);
    }
};

struct CdText {
    QList<CdTextBlock> blocks; // ascending block number
    int packCount = 0;
    int crcErrors = 0; // packs dropped because of a CRC mismatch
    bool crcIgnored = false; // the drive delivered no valid CRC at all
    QStringList warnings;

    bool isEmpty() const
    {
        return blocks.isEmpty();
    }
    // Block with the given language, otherwise block 0
    const CdTextBlock *preferredBlock(quint8 languageCode = 0x09) const;
};

std::optional<CdText> parseCdText(QByteArrayView response, QString *error = nullptr);

quint16 cdTextCrc(QByteArrayView first16Bytes); // CRC-16/CCITT, not yet inverted
QString cdTextGenreName(int code); // empty for 0, 1 and unknown codes
QString cdTextLanguageName(quint8 code);

}
