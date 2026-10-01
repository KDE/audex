/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cacheprobe.h"

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Audex::Rip
{

namespace
{

constexpr int Positions = 3; // at 30, 50 and 70 % of the audio area
constexpr int WarmUpReads = 4;

qint64 median(QList<qint64> values)
{
    if (values.isEmpty())
        return 0;
    std::sort(values.begin(), values.end());
    return values.at(values.size() / 2);
}

}

CacheDefeatCalibration calibrateCacheDefeat(SectorReader &reader, ReadableArea area, int distance, int sectors, int maxReads, int repetitions)
{
    CacheDefeatCalibration result;
    const int block = std::min(27, std::max(1, reader.maxSectorsPerRead()));
    const int span = std::max(0, area.endLba - area.firstLba - block);

    // A drive that stops answering would let each of the ~30 reads wait for
    // its timeout: the first one ends the measurement.
    const int timeoutsBefore = reader.commandTimeouts();
    const auto timedOut = [&] {
        return reader.commandTimeouts() > timeoutsBefore;
    };
    auto timedRead = [&](int lba, int count) {
        return timedOut() ? 0 : reader.read(lba, count).elapsedUs;
    };
    auto farReads = [&](int base, int n) {
        for (int i = 0; i < n && !timedOut(); ++i)
            reader.read(cacheDefeatTarget(area, base, block, i, n, distance, sectors), sectors);
    };
    const auto stop = [&] {
        result.timedOut = true;
        result.cachingDetected = false;
        result.details << u"The drive did not answer a read in time; measurement stopped."_s;
        return result;
    };

    // A drive that has just spun up, or slows down to save power, needs a few
    // reads to settle at its speed; timings taken before would be off.
    for (int i = 0; i < WarmUpReads && !timedOut(); ++i)
        reader.read(area.firstLba + std::min(span, span * 9 / 10 + i * block), block);
    if (timedOut())
        return stop();

    // Several positions: one timing can be disturbed by a speed change of the
    // drive, so the verdict is that of the majority.
    QList<qint64> colds;
    int cached = 0;
    int cachedBase = -1;
    for (int k = 0; k < Positions; ++k) {
        const int base = area.firstLba + span * (3 + 2 * k) / 10;
        farReads(base, 1); // the head away from base
        const qint64 coldUs = timedRead(base, block); // never read before: from the disc
        QList<qint64> hot;
        for (int i = 0; i < repetitions; ++i)
            hot << timedRead(base, block);
        const qint64 hotUs = median(hot);
        const bool hit = hotUs * 4 < coldUs;
        colds << coldUs;
        if (hit) {
            ++cached;
            if (cachedBase < 0)
                cachedBase = base;
        }
        result.details << u"Block of %1 sectors at LBA %2: first read %3 us, re-read %4 us%5"_s.arg(block).arg(base).arg(coldUs).arg(hotUs).arg(
            hit ? u" (cached)"_s : QString());
        if (timedOut())
            return stop();
    }
    result.cachingDetected = cached * 2 > Positions;
    if (!result.cachingDetected)
        return result;

    const qint64 coldUs = median(colds);
    const int base = cachedBase;
    for (int n = 1; n <= maxReads; n *= 2) {
        QList<qint64> times;
        for (int i = 0; i < repetitions; ++i) {
            timedRead(base, block); // cached again
            farReads(base, n);
            times << timedRead(base, block);
        }
        if (timedOut())
            return stop();
        const qint64 t = median(times);
        result.details << u"Re-read after %1 far read(s): %2 us"_s.arg(n).arg(t);
        if (t * 4 >= coldUs) {
            result.reads = n;
            return result;
        }
    }
    result.defeated = false;
    result.reads = maxReads;
    return result;
}

}
