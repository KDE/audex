/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2010 Chris Moeller
 * SPDX-FileCopyrightText: Copyright (C) 2024 Marco Nelles
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * HDCD control packet detection, ported from FFmpeg's af_hdcd filter
 * (libavfilter/af_hdcd.c): the packet format constants, the readahead
 * table and the integrate logic are derived from that BSD-3-Clause
 * licensed code. The full license text is in LICENSES/BSD-3-Clause.txt.
 */

#pragma once

#include <QByteArray>
#include <QByteArrayView>

#include <functional>

#include "core/sectorreader.h"
#include "core/toc.h"

/* HDCD (High Definition Compatible Digital) detection. Scans the least
 * significant bits of 16 bit stereo PCM for HDCD control packets. The packet
 * format matches the reverse engineered decoder in FFmpeg's af_hdcd filter.
 */
namespace Audex::Hdcd
{

struct Result {
    bool detected = false; // valid packets on both channels
    bool peakExtend = false;
    bool transientFilter = false;
    int maxGainHalfDb = 0; // strongest target gain, 0.5 dB steps
    int packets = 0;
    int errors = 0; // almost-matching packets (damaged or fake)
};

// Streaming detector; feed PCM in arbitrary chunks, result() anytime.
class Detector
{
public:
    void feed(QByteArrayView pcm);
    Result result() const;

private:
    struct Channel {
        quint64 window = 0;
        int readahead = 32;
        bool arg = false;
        quint8 control = 0;
        int packets = 0;
        int errors = 0;
        bool peakExtend = false;
        bool transientFilter = false;
        int maxGain = 0;
    };
    static void integrate(Channel &c, quint32 bit);

    Channel m_channels[2];
    QByteArray m_pending; // bytes not yet forming a complete frame
};

// Reads ~10 s from the middle of up to 3 audio tracks (fast mode) and scans
// them. The read offset does not matter: it shifts whole samples, the LSB
// stream stays intact.
Result probe(Rip::SectorReader &reader, const Cdda::Toc &toc, const std::function<bool()> &isCanceled = {});

}
