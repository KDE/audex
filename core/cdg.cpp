/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdg.h"

#include "sectorreader.h"
#include "subchannel.h"

#include <QHash>
#include <QMap>

#include <algorithm>
#include <cstring>
#include <utility>

using namespace Qt::StringLiterals;

namespace Audex::Cdda
{

namespace
{

constexpr int ScrambledPairs[3][2] = {{1, 18}, {2, 5}, {3, 23}};
constexpr int InterleaveDepth = 8; // packs
constexpr int ReadMargin = 10; // sectors around a range: shift of the sub-channel, interleave
constexpr int ChunkSectors = 64;
constexpr int ExtraReads = 3; // after two reads that differ

void swapPairs(char *pack)
{
    for (const auto &pair : ScrambledPairs)
        std::swap(pack[pair[0]], pack[pair[1]]);
}

bool isGraphicsPack(const char *pack)
{
    const int mode = pack[0] & 0x3F;
    if (mode != 0x09 && mode != 0x0A) // TV graphics, extended TV graphics
        return false;
    switch (pack[1] & 0x3F) {
    case 1: // memory preset
    case 2: // border preset
    case 6: // tile block
    case 20: // scroll preset
    case 24: // scroll copy
    case 28: // transparent color
    case 30: // color table, low
    case 31: // color table, high
    case 38: // tile block (XOR)
        return true;
    default:
        return false;
    }
}

// Raw sub-channel of [first, end): what the drive delivers, stops at the
// first command that fails
QByteArray readRange(Rip::SectorReader &reader, int first, int end, const std::function<bool()> &canceled)
{
    QByteArray out;
    for (int lba = first; lba < end && !canceled();) {
        const int n = std::min(ChunkSectors, end - lba);
        const QByteArray part = reader.readSubchannelRaw(lba, n);
        out += part.left(qsizetype(part.size() / SubcodeBytes) * SubcodeBytes);
        if (part.size() < qsizetype(n) * SubcodeBytes)
            break;
        lba += n;
    }
    return out;
}

QByteArrayView block(const QByteArray &data, int index)
{
    return QByteArrayView(data).sliced(qsizetype(index) * SubcodeBytes, SubcodeBytes);
}

bool sameRw(QByteArrayView a, QByteArrayView b)
{
    for (int i = 0; i < SubcodeBytes; ++i)
        if ((a[i] ^ b[i]) & 0x3F)
            return false;
    return true;
}

}

QByteArray qFrameFromSubcode(QByteArrayView block)
{
    QByteArray q(SubQBytes, '\0');
    for (int bit = 0; bit < SubQBytes * 8 && bit < block.size(); ++bit)
        if (block[bit] & 0x40)
            q[bit / 8] = char(q.at(bit / 8) | (0x80 >> (bit % 8)));
    return q;
}

QByteArray rwFromSubcode(QByteArrayView blocks)
{
    QByteArray rw(blocks.size(), '\0');
    for (qsizetype i = 0; i < blocks.size(); ++i)
        rw[i] = char(blocks[i] & 0x3F);
    return rw;
}

QByteArray deinterleaveCdg(QByteArrayView packs)
{
    const qsizetype count = packs.size() / CdgPackBytes;
    QByteArray out(count * CdgPackBytes, '\0');
    for (qsizetype x = 0; x < count; ++x) {
        char *pack = out.data() + x * CdgPackBytes;
        for (int i = 0; i < CdgPackBytes; ++i) {
            const qsizetype from = x + i % InterleaveDepth;
            if (from < count)
                pack[i] = packs[from * CdgPackBytes + i];
        }
        swapPairs(pack);
    }
    return out;
}

QByteArray interleaveCdg(QByteArrayView packs)
{
    const qsizetype count = packs.size() / CdgPackBytes;
    QByteArray scrambled = packs.first(count * CdgPackBytes).toByteArray();
    for (qsizetype x = 0; x < count; ++x)
        swapPairs(scrambled.data() + x * CdgPackBytes);
    QByteArray out(count * CdgPackBytes, '\0');
    for (qsizetype y = 0; y < count; ++y)
        for (int i = 0; i < CdgPackBytes; ++i) {
            const qsizetype from = y - i % InterleaveDepth;
            if (from >= 0)
                out[y * CdgPackBytes + i] = scrambled.at(from * CdgPackBytes + i);
        }
    return out;
}

int cdgGraphicsPacks(QByteArrayView packs)
{
    int n = 0;
    for (qsizetype p = 0; p + CdgPackBytes <= packs.size(); p += CdgPackBytes)
        n += isGraphicsPack(packs.data() + p) ? 1 : 0;
    return n;
}

bool detectCdg(Rip::SectorReader &reader, const Toc &toc, const std::function<bool()> &isCanceled)
{
    const int first = toc.audioStartLba();
    const int end = toc.audioEndLba();
    constexpr int Sectors = 77; // one second plus the interleave
    if (end - first < Sectors)
        return false;

    const int timeouts = reader.commandTimeouts();
    reader.setProbeMode(true);
    int graphics = 0;
    for (int k = 1; k <= 3 && graphics < 8; ++k) {
        if ((isCanceled && isCanceled()) || reader.commandTimeouts() > timeouts)
            break;
        const int lba = first + (end - first - Sectors) * k / 4;
        const QByteArray rw = rwFromSubcode(reader.readSubchannelRaw(lba, Sectors));
        graphics += std::max(cdgGraphicsPacks(rw), cdgGraphicsPacks(deinterleaveCdg(rw)));
    }
    reader.setProbeMode(false);
    return graphics >= 8;
}

CdgExtraction extractCdg(Rip::SectorReader &reader,
                         const Toc &toc,
                         int firstLba,
                         int lastLba,
                         const std::function<void(const QString &)> &message,
                         const std::function<bool()> &isCanceled)
{
    CdgExtraction result;
    const auto canceled = [&] {
        return isCanceled && isCanceled();
    };
    const auto say = [&](const QString &text) {
        if (message)
            message(text);
    };
    if (lastLba < firstLba)
        return result;

    // outside the audio area many drives do not answer, some only after a timeout
    const int readFirst = std::max(toc.audioStartLba(), firstLba - ReadMargin);
    const int readEnd = std::min(toc.audioEndLba(), lastLba + 1 + ReadMargin);
    const int sectors = readEnd - readFirst;

    // a drive that does not answer would let every further read wait for its timeout
    const int timeouts = reader.commandTimeouts();
    const auto timedOut = [&] {
        return reader.commandTimeouts() > timeouts;
    };

    say(u"Reading the CD+G sub-channel (1 of 2)..."_s);
    QByteArray first = readRange(reader, readFirst, readEnd, canceled);
    if (timedOut()) {
        result.error = u"the drive did not answer a read of the raw sub-channel in time"_s;
        return result;
    }
    if (first.isEmpty()) {
        result.error = u"the drive does not deliver the raw sub-channel"_s;
        return result;
    }
    if (canceled())
        return result;
    say(u"Reading the CD+G sub-channel (2 of 2)..."_s);
    const QByteArray second = readRange(reader, readFirst, readEnd, canceled);
    const int complete = int(std::min(first.size(), second.size()) / SubcodeBytes);
    if (complete < sectors)
        result.error = u"the sub-channel could be read up to LBA %1 only"_s.arg(readFirst + complete - 1);

    // --- the drive's shift: the Q channel of the same data tells where a block belongs ---
    QHash<int, int> shifts;
    for (int j = 0; j < complete; ++j) {
        const std::optional<SubQ> q = parseSubQ(qFrameFromSubcode(block(first, j)));
        if (q && q->adr == 1)
            ++shifts[q->absoluteLba - (readFirst + j)];
    }
    for (auto it = shifts.cbegin(); it != shifts.cend(); ++it)
        if (!result.shiftMeasured || it.value() > shifts.value(result.shift)) {
            result.shift = it.key();
            result.shiftMeasured = true;
        }

    // --- sectors whose two reads differ: read again until two reads agree ---
    QList<int> differing;
    for (int j = 0; j < complete; ++j)
        if (!sameRw(block(first, j), block(second, j)))
            differing << j;
    result.rereadSectors = int(differing.size());
    for (const int j : std::as_const(differing)) {
        if (canceled() || timedOut())
            break;
        QList<QByteArray> reads{block(first, j).toByteArray(), block(second, j).toByteArray()};
        bool agreed = false;
        for (int attempt = 0; attempt < ExtraReads && !agreed; ++attempt) {
            const QByteArray again = reader.readSubchannelRaw(readFirst + j, 1);
            if (again.size() < SubcodeBytes)
                break;
            for (const QByteArray &earlier : std::as_const(reads)) {
                if (sameRw(earlier, again)) {
                    std::memcpy(first.data() + qsizetype(j) * SubcodeBytes, again.constData(), SubcodeBytes);
                    agreed = true;
                    break;
                }
            }
            reads << again;
        }
        if (!agreed)
            ++result.unresolvedSectors;
    }

    // --- the R-W stream of [firstLba, lastLba + 2], in disc order ---
    const int streamSectors = lastLba - firstLba + 1 + 2; // the interleave reaches 7 packs ahead
    QByteArray stream(qsizetype(streamSectors) * SubcodeBytes, '\0');
    for (int s = 0; s < streamSectors; ++s) {
        const int j = firstLba + s - result.shift - readFirst; // requested with the drive's shift
        if (j >= 0 && j < complete)
            std::memcpy(stream.data() + qsizetype(s) * SubcodeBytes, first.constData() + qsizetype(j) * SubcodeBytes, SubcodeBytes);
    }
    stream = rwFromSubcode(stream);

    // most drives deliver the packs as on the disc, a few de-interleave them
    const QByteArray deinterleaved = deinterleaveCdg(stream);
    const int asDelivered = cdgGraphicsPacks(stream);
    const int afterDeinterleaving = cdgGraphicsPacks(deinterleaved);
    result.deinterleavedByDrive = asDelivered > afterDeinterleaving;
    const QByteArray &packs = result.deinterleavedByDrive ? stream : deinterleaved;
    result.graphicsPacks = std::max(asDelivered, afterDeinterleaving);
    result.found = result.graphicsPacks > 0;
    result.packs = packs.first(qsizetype(lastLba - firstLba + 1) * SubcodeBytes);
    return result;
}

}
