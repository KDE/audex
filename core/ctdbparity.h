/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * SPDX-FileCopyrightText: Copyright (C) 2008-2025 Gregory S. Chudov
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// Recovery data for the CUETools Database (CTDB). Based on CUETools
// (GPL-2.0-or-later). The table-driven parity, syndrome conversion,
// cross-pressing syndrome shifts, and offset search mirror the original
// CUETools code to ensure compatibility with database-stored syndromes.
// The Reed-Solomon decoder is an independent, textbook implementation
// using Berlekamp-Massey, Chien, and Forney.

// The audio from index 01 of the first audio track to the end of the last one
// is split into rows of Stride 16 bit words (10 sectors). Without the first
// row and the last one or two, every column is a Reed-Solomon code word over
// GF(2^16) with up to 16 parity symbols: up to 8 wrong words per column can
// be corrected, e.g. about one second of damaged audio in one piece.

#include <QByteArray>
#include <QByteArrayView>
#include <QList>

#include <array>
#include <optional>
#include <vector>

#include "core/cdda.h"
#include "core/checksums.h"

namespace Audex::Ctdb
{

inline constexpr int StrideSamples = 10 * Cdda::SamplesPerSector;
inline constexpr int ShiftRange = StrideSamples - 1; // offsets between pressings (samples)
inline constexpr int Stride = 2 * StrideSamples; // 16 bit words per row
inline constexpr int MaxParity = 16;

using ColumnSyndromes = std::array<quint16, MaxParity>;

struct Correction {
    qint64 word = 0; // 16 bit word in the stream given to Parity::update()
    quint16 mask = 0; // XOR mask that corrects it
};

struct ShiftMatch {
    int shift = 0; // the database's sample i is our sample i + shift
    bool errors = false; // the rip differs from the database at this shift
};

class Parity
{
public:
    // The stream given to update() reaches index 01 of the first audio track
    // after offsetWords 16 bit words; words is the length of the CTDB audio.
    Parity(qint64 offsetWords, qint64 words);

    bool isValid() const; // long enough and not too long
    void update(QByteArrayView pcm); // any chunk size
    bool isComplete() const;

    int rows() const
    {
        return m_rows;
    }

    // After completion: the CTDB CRC of the rip moved by shift samples, and
    // the syndromes of a column at that shift
    quint32 crc(int shift) const;
    ColumnSyndromes syndromes(int column, int shift) const;

    // Syndromes of all columns at shift 0 in the layout of the database's
    // recovery data: npar blocks of Stride little endian words
    QByteArray syndromeData(int npar) const;

    // The shift at which the rip matches or can be matched to a database entry
    // from the syndromes of its first column (CUETools: FindOffset)
    std::optional<ShiftMatch> findShift(const QList<quint16> &column0, quint32 crc) const;

    // The corrections that turn the rip into the database entry, given its
    // recovery data (syndromeData() layout, at least npar blocks). Nothing if
    // the damage is too large or the result does not match crc (CUETools:
    // VerifyParity).
    std::optional<QList<Correction>> corrections(QByteArrayView data, int npar, int shift, quint32 crc) const;

    // Old database entries carry 8 parity words of the first column instead
    // of its syndromes
    static QList<quint16> syndromesFromParity(const QList<quint16> &parity);

private:
    void feed(QByteArrayView data, qint64 at);
    void finish();
    quint16 leadWord(qint64 index) const;
    quint16 tailWord(qint64 index) const;

    qint64 m_offset; // bytes
    qint64 m_words;
    int m_rows = 0;
    qint64 m_regionBegin = 0; // bytes, first data row
    qint64 m_regionEnd = 0;
    qint64 m_tailBegin = 0; // bytes, row rows() - 1
    qint64 m_position = 0; // bytes seen
    bool m_complete = false;

    QByteArray m_lead; // rows -1 and 0
    QByteArray m_tail; // last data row and the rest
    std::vector<quint64> m_parity; // Stride x 4 lanes of 4 parity words
    std::vector<ColumnSyndromes> m_syndromes;
    Rip::Crc32 m_crc;
    QList<quint32> m_crcs; // index shift + ShiftRange
    int m_pending = -1; // byte of a word split between two chunks
};

}
