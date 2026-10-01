/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "core/cdda.h"
#include "core/checksums.h"

#include <QRandomGenerator>

using namespace Audex;
using namespace Audex::Rip;

AUDEX_TEST("sliding CRC-32 equals the CRC of the moved window")
{
    constexpr int S = Cdda::BytesPerSample;
    constexpr int range = 300;
    constexpr qint64 first = 1000; // window in samples
    constexpr qint64 samples = 5000;
    QByteArray pcm(qsizetype(first + samples + range + 10) * S, '\0');
    QRandomGenerator rng(3);
    for (char &c : pcm)
        c = char(rng.bounded(256));

    const auto window = [&](qint64 from) {
        return QByteArrayView(pcm).sliced(from * S, samples * S);
    };
    const QList<quint32> crcs = slidingCrc32(Crc32::compute(window(first)),
                                             samples * S,
                                             QByteArrayView(pcm).sliced((first - range) * S, 2 * range * S),
                                             QByteArrayView(pcm).sliced((first + samples - range) * S, 2 * range * S),
                                             range);
    AUDEX_EQUAL(t, crcs.size(), 2 * range + 1);
    for (int shift : {-range, -range + 1, -1, 0, 1, 137, range})
        AUDEX_EQUAL(t, qint64(crcs.value(shift + range)), qint64(Crc32::compute(window(first + shift))));
}
