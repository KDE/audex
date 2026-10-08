/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArrayView>
#include <QList>
#include <QtGlobal>

// All calculators accept arbitrary chunk sizes; the result does not depend on
// how the stream was split.

namespace Audex::Rip
{

// Standard CRC-32 (IEEE 802.3, as zlib)
class Crc32
{
public:
    void reset()
    {
        m_crc = 0xFFFFFFFFu;
    }
    void update(QByteArrayView data);
    quint32 value() const
    {
        return ~m_crc;
    }

    static quint32 compute(QByteArrayView data);

private:
    quint32 m_crc = 0xFFFFFFFFu;
};

// CRC-32 of the samples [skipFirst, samples - skipLast) of a track, as the
// CUETools database (CTDB) uses it for the first and last track of a disc.
class ClippedCrc32
{
public:
    ClippedCrc32() = default;
    ClippedCrc32(qint64 samples, qint64 skipFirst, qint64 skipLast);

    void update(QByteArrayView data);
    quint32 value() const
    {
        return m_crc.value();
    }
    qint64 samples() const // length of the window
    {
        return (m_to - m_from) / 4;
    }

private:
    qint64 m_from = 0; // bytes
    qint64 m_to = 0;
    qint64 m_position = 0;
    Crc32 m_crc;
};

// CRC-32 of A followed by B, from crc(A), crc(B) and the length of B in bytes
// (as zlib's crc32_combine).
quint32 crc32Combine(quint32 crcA, quint32 crcB, qint64 bytesB);

// CRC-32 of a message of `length` bytes after `delta` has been XORed into it
// at byte `position`, from the CRC of the original message
quint32 crc32Xor(quint32 crc, qint64 length, qint64 position, QByteArrayView delta);

// CRC-32 of a window of `length` bytes of PCM moved by -range..range samples,
// from its CRC at shift 0 and the PCM around its first and its end sample
// (head and tail: samples [-range, range) relative to these positions).
// result[shift + range]; empty if head or tail are too short.
QList<quint32> slidingCrc32(quint32 crc, qint64 length, QByteArrayView head, QByteArrayView tail, int range);

// CRC-32 over all 16 bit sample values that are not zero
// ("W/O NULL" in CUETools logs). Samples are little-endian 16 bit.
class Crc32NoNull
{
public:
    void reset();
    void update(QByteArrayView data);
    quint32 value() const
    {
        return ~m_crc;
    }

private:
    quint32 m_crc = 0xFFFFFFFFu;
    char m_pending = 0;
    bool m_hasPending = false;
};

// AccurateRip track checksums (v1, v2 and the frame 450 CRC used for offset
// detection). Definition as in the AccurateRip reference code:
//
//   multiplier m runs from 1 to N (N = samples of the track)
//   first track: only samples with m >= 5*588 are included
//   last track:  only samples with m <= N - 5*588 are included
//   v1 += lo32(sample * m),  v2 += lo32(sample * m) + hi32(sample * m)
//
// "first"/"last" refer to the first/last audio track of the disc.
class AccurateRipChecksum
{
public:
    AccurateRipChecksum() = default;
    AccurateRipChecksum(qint64 trackSamples, bool firstTrack, bool lastTrack);

    void update(QByteArrayView data);

    quint32 v1() const
    {
        return m_v1;
    }
    quint32 v2() const
    {
        return m_v2;
    }
    quint32 frame450() const
    {
        return m_frame450;
    }

private:
    void processSample(quint32 sample);

    qint64 m_checkFrom = 1;
    qint64 m_checkTo = 0;
    qint64 m_multiplier = 1;
    quint32 m_v1 = 0;
    quint32 m_v2 = 0;
    quint32 m_frame450 = 0;
    char m_pending[4] = {};
    int m_pendingLength = 0;
};

// Largest pressing or read offset AccurateRip matching looks at (samples).
inline constexpr int AccurateRipShiftRange = 5 * 588 - 1;

// Frame 450 checksums for every shift in [-range, range], result[shift + range].
// pcm starts `range` samples before sector 450 of the track and holds
// 588 + 2 * range samples. Shift s means the database pressing's sample i is
// our sample i + s.
QList<quint32> frame450Checksums(QByteArrayView pcm, int range = AccurateRipShiftRange);

// Peak level of 16 bit stereo PCM, in percent of full scale.
class PeakMeter
{
public:
    void update(QByteArrayView data);
    double percent() const
    {
        return m_peak * 100.0 / 32768.0;
    }

private:
    int m_peak = 0;
    char m_pending = 0;
    bool m_hasPending = false;
};

}
