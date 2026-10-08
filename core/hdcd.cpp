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

#include "hdcd.h"

#include "core/ripengine.h"

namespace Audex::Hdcd
{

namespace
{
// distance to the next sync word candidate, indexed by the low byte of the
// LSB window (same table as FFmpeg's readaheadtab)
const quint8 readaheadtab[256] = {
    0x03, 0x02, 0x01, 0x01, 0x1f, 0x1e, 0x1f, 0x11, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x10, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e,
    0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x0f, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1b, 0x1f, 0x1e, 0x1f, 0x1d,
    0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x0e, 0x19, 0x07, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e,
    0x1f, 0x1b, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1a, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c,
    0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1b, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x0d, 0x18, 0x20, 0x06, 0x1e,
    0x1f, 0x12, 0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1b, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d,
    0x1f, 0x1e, 0x1f, 0x1a, 0x08, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1b, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e,
    0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x19, 0x1f, 0x13, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1b,
    0x09, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c, 0x1f, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1a, 0x14, 0x1e, 0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1c, 0x0a, 0x1e,
    0x1f, 0x1d, 0x1f, 0x1e, 0x1f, 0x1b, 0x15, 0x1e, 0x1f, 0x1d, 0x0b, 0x1e, 0x1f, 0x1c, 0x16, 0x1e, 0x0c, 0x1d, 0x17, 0x1e, 0x1f, 0x20};
} // namespace

void Detector::integrate(Channel &c, quint32 bit)
{
    c.window = (c.window << 1) | bit;
    if (--c.readahead > 0)
        return;
    const quint32 wbits = quint32(c.window ^ (c.window >> 5) ^ (c.window >> 23));
    if (c.arg) {
        bool ok = false;
        quint8 control = 0;
        if ((wbits & 0x0fa00500) == 0x0fa00500) { // packet A: 8-bit code
            if ((wbits & 0xc8) == 0) {
                control = quint8((wbits & 255) + (wbits & 7));
                ok = true;
            }
        } else if ((wbits & 0xa0060000) == 0xa0060000) { // packet B: code + XOR check
            if (((wbits ^ (~wbits >> 8 & 255)) & 0xffff00ff) == 0xa0060000) {
                control = quint8(wbits >> 8 & 255);
                ok = true;
            }
        }
        if (ok) {
            c.control = control;
            ++c.packets;
            c.peakExtend = c.peakExtend || (control & 16);
            c.transientFilter = c.transientFilter || (control & 32);
            c.maxGain = qMax(c.maxGain, int(control & 15));
        } else {
            ++c.errors;
        }
        c.arg = false;
    }
    if (wbits == 0x7e0fa005 || wbits == 0x7e0fa006) { // packet prefix
        c.readahead = int(wbits & 3) * 8;
        c.arg = true;
    } else {
        c.readahead = wbits ? readaheadtab[wbits & 0xff] : 31; // ffwd over silence
    }
}

void Detector::feed(QByteArrayView pcm)
{
    m_pending.append(pcm.constData(), pcm.size());
    const char *d = m_pending.constData();
    const qsizetype frames = m_pending.size() / Cdda::BytesPerSample;
    for (qsizetype i = 0; i < frames; ++i, d += Cdda::BytesPerSample) {
        integrate(m_channels[0], quint8(d[0]) & 1);
        integrate(m_channels[1], quint8(d[2]) & 1);
    }
    m_pending.remove(0, frames * Cdda::BytesPerSample);
}

Result Detector::result() const
{
    Result r;
    r.packets = m_channels[0].packets + m_channels[1].packets;
    r.errors = m_channels[0].errors + m_channels[1].errors;
    r.detected = m_channels[0].packets > 0 && m_channels[1].packets > 0;
    r.peakExtend = m_channels[0].peakExtend || m_channels[1].peakExtend;
    r.transientFilter = m_channels[0].transientFilter || m_channels[1].transientFilter;
    r.maxGainHalfDb = qMax(m_channels[0].maxGain, m_channels[1].maxGain);
    return r;
}

Result probe(Rip::SectorReader &reader, const Cdda::Toc &toc, const std::function<bool()> &isCanceled)
{
    constexpr int snippetSectors = 750; // ~10 s, thousands of control packets
    Detector detector;
    const QList<int> audio = toc.audioTrackNumbers();
    for (int i = 0; i < audio.size() && i < 3; ++i) {
        const Cdda::Track *t = toc.track(audio.at(i));
        if (!t || t->sectorCount() < snippetSectors)
            continue;
        Rip::Segment segment;
        segment.trackNumber = t->number;
        segment.firstLba = t->firstLba + t->sectorCount() / 2;
        segment.lastLba = segment.firstLba + snippetSectors - 1;
        segment.accurateRip = false;
        Rip::RipOptions options;
        options.mode = Rip::ReadMode::Fast;
        options.cacheDefeat = false;
        Rip::RipCallbacks callbacks;
        callbacks.write = [&detector](int, QByteArrayView data) {
            detector.feed(data);
            return true;
        };
        callbacks.isCanceled = isCanceled;
        Rip::RipEngine engine(reader, {toc.audioStartLba(), toc.audioEndLba()}, options, callbacks);
        if (!engine.run({segment}).completed)
            break;
    }
    return detector.result();
}

}
