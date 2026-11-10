/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivefeatures.h"

#include "core/cacheprobe.h"
#include "core/cdg.h"
#include "core/subchannel.h"

#include <QCoreApplication>
#include <QHash>

#include <algorithm>
#include <optional>

using namespace Qt::StringLiterals;

namespace Audex::Rip
{

namespace
{

struct Tr {
    Q_DECLARE_TR_FUNCTIONS(Audex::DriveFeatures)
};

using Cdda::BytesPerSample;
using Cdda::SamplesPerSector;
using Cdda::SectorBytes;
using Cdda::SubcodeBytes;
using Cdda::SubQBytes;

constexpr int ProbeSectors = 8; // block the checks work on
constexpr int LeadSectors = 2; // read in front of it to find a misalignment
constexpr int NeedleSectors = 2; // part of a block that is searched for
constexpr int MaxShiftSamples = 2 * SamplesPerSector;
constexpr int DefeatDistance = 5000;
constexpr int Positions = 3; // probe positions per check
constexpr int C2Reads = 6;

// sector counts a drive or its USB bridge may be limited to
const QList<int> BurstCandidates{27, 26, 25, 24, 20, 16, 13, 10, 8, 6, 4, 2, 1};

bool isSilent(QByteArrayView data)
{
    return std::all_of(data.cbegin(), data.cend(), [](char c) {
        return c == 0;
    });
}

// Where does `needle` sit in `haystack`? nominalSamples is where a drive that
// hits the requested position exactly would put it; the result is by how many
// samples this drive missed it. Nothing: the data does not match at all.
std::optional<int> alignShift(QByteArrayView haystack, qint64 nominalSamples, QByteArrayView needle)
{
    const auto matches = [&](int shift) {
        const qint64 at = (nominalSamples + shift) * BytesPerSample;
        if (at < 0 || at + needle.size() > haystack.size())
            return false;
        return memcmp(haystack.data() + at, needle.data(), size_t(needle.size())) == 0;
    };
    for (int shift = 0; shift <= MaxShiftSamples; ++shift) {
        if (matches(shift))
            return shift;
        if (shift > 0 && matches(-shift))
            return -shift;
    }
    return std::nullopt;
}

// Commands a drive or its USB bridge may not answer at all are sent with a
// short timeout and without the single-sector retries of a failed read.
class Probing
{
public:
    explicit Probing(SectorReader &reader)
        : m_reader(reader)
    {
        m_reader.setProbeMode(true);
    }
    ~Probing()
    {
        m_reader.setProbeMode(false);
    }
    Probing(const Probing &) = delete;
    Probing &operator=(const Probing &) = delete;

private:
    SectorReader &m_reader;
};

// State the checks share: the drive, the audio area and what has been measured
// about it so far.
struct Context {
    SectorReader &reader;
    ReadableArea area;
    std::function<bool()> isCanceled;
    int burst = ProbeSectors; // largest read command used by the checks
    int defeatReads = 4; // until the cache check knows better
    int goodLba = -1; // a position that was read without problems
    int answeredTimeouts = 0; // reader.commandTimeouts() when the drive last answered
    bool positionTimedOut = false; // position() got no answer
    bool stalled = false; // the drive stopped answering

    bool canceled() const
    {
        return isCanceled && isCanceled();
    }

    bool timedOutSince(int timeouts) const
    {
        return reader.commandTimeouts() > timeouts;
    }

    // A command that times out is aborted by the kernel, often with a reset of
    // the drive. After that the drive has to answer a plain read again before
    // the next check: one that does not would make every further read wait
    // for the timeout, and the test would seem to hang.
    bool answering()
    {
        if (stalled)
            return false;
        if (!timedOutSince(answeredTimeouts))
            return true;
        for (int attempt = 0; attempt < 2 && !canceled(); ++attempt) {
            const int before = reader.commandTimeouts();
            SectorReadResult r;
            {
                const Probing probing(reader); // a drive that is gone does not keep us 30 s
                r = reader.read(goodLba >= 0 ? goodLba : area.firstLba, 1);
            }
            if (r.allOk()) {
                answeredTimeouts = reader.commandTimeouts();
                return true;
            }
            if (timedOutSince(before))
                break; // no second wait; a plain error (the reset reported once) gets another try
        }
        stalled = !canceled();
        return false;
    }

    int block() const
    {
        return std::clamp(ProbeSectors, 1, burst);
    }

    // Read somewhere far away so that the next read reaches the disc again.
    void defeatCache(int lba, int count)
    {
        const int reads = std::max(1, defeatReads);
        for (int i = 0; i < reads && !canceled(); ++i)
            reader.read(cacheDefeatTarget(area, lba, count, i, reads, DefeatDistance, 1), 1);
    }

    // A position with audio in it: silence would let every comparison succeed.
    int position(int index)
    {
        const int span = area.endLba - area.firstLba - ProbeSectors - 2 * LeadSectors;
        if (span <= 0)
            return -1;
        int lba = area.firstLba + LeadSectors + span * (index + 1) / (Positions + 1);
        positionTimedOut = false;
        for (int tries = 0; tries < 6 && !canceled(); ++tries) {
            const int before = reader.commandTimeouts();
            const SectorReadResult r = reader.read(lba, block());
            if (r.allOk() && !isSilent(r.audio)) {
                goodLba = lba;
                return lba;
            }
            if (timedOutSince(before)) {
                positionTimedOut = true; // another position would only wait again
                return -1;
            }
            lba += 500;
            if (lba + ProbeSectors + LeadSectors >= area.endLba)
                lba = area.firstLba + LeadSectors;
        }
        return -1;
    }

    QString noPosition() const
    {
        return positionTimedOut ? Tr::tr("not tested: the drive did not answer in time") : Tr::tr("not tested: no usable position on this disc");
    }
};

// ---- the checks ------------------------------------------------------------

// A read command the drive (or a USB bridge in front of it) cannot do fails as
// a whole; the MMC reader then falls back to single sectors, which is slow.
// Oversized transfers can also return wrong data, so the block is compared
// with a second, small read of its middle.
bool burstWorks(Context &ctx, int lba, int sectors)
{
    ctx.defeatCache(lba, sectors);
    SectorReadResult big;
    {
        const Probing probing(ctx.reader); // a size the bridge cannot do may not be answered
        big = ctx.reader.read(lba, sectors);
    }
    if (!big.error.isEmpty() || !big.allOk() || big.splitIntoSingleReads)
        return false;
    if (sectors < NeedleSectors + 2)
        return true;
    const int needleLba = lba + sectors / 2 - 1;
    ctx.defeatCache(needleLba, NeedleSectors);
    const SectorReadResult needle = ctx.reader.read(needleLba, NeedleSectors);
    if (!needle.allOk() || isSilent(needle.audio))
        return true; // nothing to compare against
    return alignShift(big.audio, qint64(needleLba - lba) * SamplesPerSector, needle.audio).has_value();
}

FeatureCheck checkBurst(Context &ctx, DriveFeatures &f)
{
    FeatureCheck c;
    QList<int> positions;
    for (int i = 0; i < 2; ++i) {
        const int p = ctx.position(i);
        if (p >= 0)
            positions << p;
    }
    if (positions.isEmpty()) {
        c.summary = ctx.noPosition();
        return c;
    }

    for (const int sectors : BurstCandidates) {
        if (ctx.canceled())
            break;
        bool ok = true;
        for (const int p : std::as_const(positions))
            ok = ok && burstWorks(ctx, p, sectors);
        if (!ok) {
            c.details << Tr::tr("%1 sectors per command: failed").arg(sectors);
            continue;
        }
        c.details << Tr::tr("%1 sectors per command: ok").arg(sectors);
        f.burstSectors = sectors;
        ctx.burst = sectors;
        c.verdict = sectors >= BurstCandidates.constFirst() ? Feature::Yes : Feature::No;
        c.summary = sectors >= BurstCandidates.constFirst() ? Tr::tr("up to %n sector(s) per command (no limit found)", nullptr, sectors)
                                                            : Tr::tr("up to %n sector(s) per command", nullptr, sectors);
        return c;
    }
    c.summary = Tr::tr("not tested");
    return c;
}

FeatureCheck checkCache(Context &ctx, DriveFeatures &f)
{
    FeatureCheck c;
    const CacheDefeatCalibration cal = calibrateCacheDefeat(ctx.reader, ctx.area, DefeatDistance);
    c.details = cal.details;
    if (cal.timedOut) {
        c.summary = Tr::tr("not tested: the drive did not answer in time");
        return c;
    }
    f.caching = cal.cachingDetected ? Feature::Yes : Feature::No;
    c.verdict = f.caching;
    if (!cal.cachingDetected) {
        f.cacheDefeatReads = 0;
        c.summary = Tr::tr("none: every read reaches the disc");
        return c;
    }
    f.cacheDefeatReads = cal.reads;
    ctx.defeatReads = cal.reads;
    c.summary = cal.defeated ? Tr::tr("caches audio, %n far read(s) flush it", nullptr, cal.reads)
                             : Tr::tr("caches audio, %n far read(s) did not flush it", nullptr, cal.reads);
    return c;
}

// Two reads of the same sectors must return the same samples. A drive without
// accurate stream misses the position by a few samples after every seek.
FeatureCheck checkStream(Context &ctx, DriveFeatures &f)
{
    FeatureCheck c;
    int aligned = 0;
    int shifted = 0;
    int unusable = 0;
    const int window = std::min(ctx.burst, ProbeSectors + 2 * LeadSectors);
    const int inner = std::max(1, window - 2 * LeadSectors);

    for (int i = 0; i < Positions && !ctx.canceled(); ++i) {
        const int p = ctx.position(i);
        if (p < 0)
            continue;
        ctx.defeatCache(p, inner);
        const SectorReadResult a = ctx.reader.read(p, inner);
        ctx.defeatCache(p - LeadSectors, window);
        const SectorReadResult b = ctx.reader.read(p - LeadSectors, window);
        if (!a.allOk() || !b.allOk() || isSilent(a.audio)) {
            ++unusable;
            continue;
        }
        const QByteArrayView needle = QByteArrayView(a.audio).sliced(0, qsizetype(std::min(inner, NeedleSectors)) * SectorBytes);
        const std::optional<int> shift = alignShift(b.audio, qint64(LeadSectors) * SamplesPerSector, needle);
        if (!shift) {
            ++unusable;
            c.details << Tr::tr("LBA %1: the two reads have no samples in common").arg(p);
        } else if (*shift == 0) {
            ++aligned;
            c.details << Tr::tr("LBA %1: both reads deliver the same samples").arg(p);
        } else {
            ++shifted;
            f.jitterSamples = std::max(f.jitterSamples, std::abs(*shift));
            c.details << Tr::tr("LBA %1: the second read is shifted by %2 samples").arg(p).arg(*shift);
        }
    }

    if (shifted > 0) {
        f.accurateStream = Feature::No;
        c.summary = Tr::tr("no: reads are up to %1 samples off after a seek; Audex cannot compensate for this, rips with this drive are not reliable")
                        .arg(f.jitterSamples);
    } else if (aligned >= 2) {
        f.accurateStream = Feature::Yes;
        c.summary = Tr::tr("yes: repeated reads line up exactly");
    } else {
        c.summary = Tr::tr("not tested: %1 of %2 positions could not be compared").arg(unusable).arg(Positions);
    }
    c.verdict = f.accurateStream;
    return c;
}

// The drive is asked for the same block several times. Data it got wrong must
// carry C2 pointers, otherwise they cannot be trusted as an error check.
FeatureCheck checkC2(Context &ctx, DriveFeatures &f)
{
    FeatureCheck c;
    f.c2Supported = ctx.reader.supportsC2();
    if (!f.c2Supported) {
        f.c2Reliable = Feature::No;
        c.verdict = Feature::No;
        c.summary = Tr::tr("not supported by this drive");
        return c;
    }
    const int p = ctx.position(1);
    if (p < 0) {
        c.summary = ctx.noPosition();
        return c;
    }

    // C2 pointers only for the reads that are compared, not for the far
    // reads of the cache defeat
    const auto readWithC2 = [&ctx](int lba, int count) {
        const Probing probing(ctx.reader);
        ctx.reader.setC2Enabled(true);
        SectorReadResult r = ctx.reader.read(lba, count);
        ctx.reader.setC2Enabled(false);
        return r;
    };

    QList<QByteArray> copies;
    QList<int> flags;
    for (int i = 0; i < C2Reads && !ctx.canceled(); ++i) {
        ctx.defeatCache(p, ctx.block());
        if (ctx.canceled())
            break;
        const int timeouts = ctx.reader.commandTimeouts();
        const SectorReadResult r = readWithC2(p, ctx.block());
        const bool timedOut = ctx.timedOutSince(timeouts);
        if ((i == 0 || timedOut) && !r.allOk()) {
            // The block was just read without C2 pointers (position()): it is
            // the read command with C2 pointers the drive or its USB bridge
            // cannot do. A rip would fail (or wait) on every block.
            f.c2Supported = false;
            f.c2Reliable = Feature::No;
            c.verdict = Feature::No;
            c.summary = timedOut ? Tr::tr("advertised by the drive, but reads with C2 pointers get no answer")
                                 : Tr::tr("advertised by the drive, but reads with C2 pointers fail");
            c.details << Tr::tr("LBA %1: read of %n sector(s) with C2 pointers: %2", nullptr, ctx.block())
                             .arg(p)
                             .arg(r.error.isEmpty() ? Tr::tr("no data") : r.error);
            return c;
        }
        if (!r.allOk()) {
            // a failed read is no copy (its sectors are zero-filled); another
            // attempt would most likely only wait for the same error again
            c.details << Tr::tr("LBA %1: read with C2 pointers failed: %2").arg(p).arg(r.error.isEmpty() ? Tr::tr("no data") : r.error);
            break;
        }
        int c2 = 0;
        for (const SectorStatus &s : r.sectors)
            c2 += s.c2Errors;
        copies << r.audio;
        flags << c2;
    }
    if (copies.size() < 2) {
        c.summary = ctx.canceled() ? Tr::tr("not tested: stopped") : Tr::tr("not tested: the block could not be read repeatedly");
        return c;
    }

    // the copy that turns up most often is the correct one
    QHash<QByteArray, int> votes;
    for (const QByteArray &copy : std::as_const(copies))
        ++votes[copy];
    QByteArray correct = copies.constFirst();
    for (auto it = votes.cbegin(); it != votes.cend(); ++it)
        if (it.value() > votes.value(correct))
            correct = it.key();

    int faulty = 0;
    int faultyFlagged = 0;
    int cleanFlagged = 0;
    for (qsizetype i = 0; i < copies.size(); ++i) {
        if (copies.at(i) != correct) {
            ++faulty;
            if (flags.at(i) > 0)
                ++faultyFlagged;
        } else if (flags.at(i) > 0) {
            ++cleanFlagged;
        }
    }
    c.details << Tr::tr("%n read(s) of LBA %1: %2 faulty, %3 of them flagged by C2", nullptr, int(copies.size())).arg(p).arg(faulty).arg(faultyFlagged);
    if (cleanFlagged > 0)
        c.details << Tr::tr("%n read(s) carried C2 pointers although the data was correct", nullptr, cleanFlagged);

    if (faulty == 0) {
        c.summary = cleanFlagged > 0 ? Tr::tr("supported, but C2 errors are reported for correct data")
                                     : Tr::tr("supported; this disc produced no read errors, so nothing could be checked");
    } else if (faultyFlagged == faulty) {
        f.c2Reliable = Feature::Yes;
        c.summary = Tr::tr("supported and reliable: every faulty read was flagged");
    } else {
        f.c2Reliable = Feature::No;
        c.summary = Tr::tr("unreliable: %1 of %2 faulty reads were not flagged").arg(faulty - faultyFlagged).arg(faulty);
    }
    c.verdict = f.c2Reliable;
    return c;
}

FeatureCheck checkOverread(Context &ctx, DriveFeatures &f)
{
    FeatureCheck c;
    const auto probe = [&](int lba, bool leadIn) {
        SectorReadResult r;
        {
            const Probing probing(ctx.reader);
            r = ctx.reader.read(lba, 1);
        }
        const bool ok = r.error.isEmpty() && r.allOk();
        if (!ok)
            c.details << (leadIn ? Tr::tr("lead-in (LBA %1): %2") : Tr::tr("lead-out (LBA %1): %2")).arg(lba).arg(r.error);
        else if (isSilent(r.audio))
            c.details << (leadIn ? Tr::tr("lead-in (LBA %1): readable, digital silence") : Tr::tr("lead-out (LBA %1): readable, digital silence")).arg(lba);
        else
            c.details << (leadIn ? Tr::tr("lead-in (LBA %1): readable") : Tr::tr("lead-out (LBA %1): readable")).arg(lba);
        return ok ? Feature::Yes : Feature::No;
    };

    if (ctx.area.firstLba == 0)
        f.leadIn = probe(-1, true);
    else
        c.details << Tr::tr("lead-in not tested: the audio area does not start at the beginning of the disc");
    if (ctx.canceled())
        return c;
    if (ctx.answering())
        f.leadOut = probe(ctx.area.endLba, false);
    else if (ctx.stalled)
        c.details << Tr::tr("lead-out not tested: the drive stopped answering after the lead-in read");

    if (f.leadIn == Feature::Yes && f.leadOut == Feature::Yes) {
        c.verdict = Feature::Yes;
        c.summary = Tr::tr("lead-in and lead-out can be read");
    } else if (f.leadOut == Feature::Yes) {
        c.verdict = Feature::Yes;
        c.summary = Tr::tr("only the lead-out can be read");
    } else if (f.leadIn == Feature::Yes) {
        c.verdict = Feature::Yes;
        c.summary = Tr::tr("only the lead-in can be read");
    } else {
        c.verdict = Feature::No;
        c.summary = Tr::tr("not possible: the drive refuses to read outside the audio area");
    }
    return c;
}

FeatureCheck checkSubchannel(Context &ctx, DriveFeatures &f)
{
    FeatureCheck c;
    const int p = ctx.position(1);
    if (p < 0) {
        c.summary = ctx.noPosition();
        return c;
    }
    constexpr int Frames = 4; // one command
    const int timeouts = ctx.reader.commandTimeouts();
    QByteArray q;
    {
        const Probing probing(ctx.reader);
        q = ctx.reader.readSubchannelQ(p, Frames);
    }
    if (q.size() < SubQBytes) {
        f.subchannelQ = Feature::No;
        c.verdict = Feature::No;
        if (ctx.timedOutSince(timeouts)) {
            c.summary = Tr::tr("not delivered: the drive does not answer reads of the Q sub-channel");
            c.details << Tr::tr("READ CD with formatted Q sub-channel: no answer in time (timeout)");
        } else {
            c.summary = Tr::tr("not delivered by this drive");
            c.details << Tr::tr("READ CD with formatted Q sub-channel returned no data");
        }
        return c;
    }

    int valid = 0;
    int positional = 0;
    for (qsizetype i = 0; i + SubQBytes <= q.size(); i += SubQBytes) {
        const std::optional<Cdda::SubQ> frame = Cdda::parseSubQ(QByteArrayView(q).sliced(i, SubQBytes));
        if (!frame)
            continue;
        ++valid;
        if (frame->adr == 1 && std::abs(frame->absoluteLba - (p + int(i / SubQBytes))) <= 4)
            ++positional;
    }
    c.details << Tr::tr("%1 of %2 frames valid, %3 with the requested position").arg(valid).arg(q.size() / SubQBytes).arg(positional);
    f.subchannelQ = positional > 0 ? Feature::Yes : Feature::No;
    c.verdict = f.subchannelQ;
    c.summary = positional > 0 ? Tr::tr("delivered: pregaps, indexes and ISRCs can be read") : Tr::tr("delivered, but the frames are unusable");
    return c;
}

// The raw P-W sub-channel carries CD+G graphics in R-W; its Q channel tells
// by how many sectors the drive delivers it off.
FeatureCheck checkRw(Context &ctx, DriveFeatures &f)
{
    FeatureCheck c;
    const int p = ctx.position(1);
    if (p < 0) {
        c.summary = ctx.noPosition();
        return c;
    }
    constexpr int Blocks = 16; // one command
    const int timeouts = ctx.reader.commandTimeouts();
    QByteArray raw;
    {
        const Probing probing(ctx.reader);
        raw = ctx.reader.readSubchannelRaw(p, Blocks);
    }
    if (raw.size() < SubcodeBytes) {
        f.rwSubchannel = Feature::No;
        c.verdict = Feature::No;
        if (ctx.timedOutSince(timeouts)) {
            c.summary = Tr::tr("not delivered: the drive does not answer reads of the raw sub-channel");
            c.details << Tr::tr("READ CD with raw P-W sub-channel: no answer in time (timeout)");
        } else {
            c.summary = Tr::tr("not delivered by this drive: CD+G graphics cannot be read");
            c.details << Tr::tr("READ CD with raw P-W sub-channel returned no data");
        }
        return c;
    }

    const int blocks = int(raw.size() / SubcodeBytes);
    QHash<int, int> shifts;
    int valid = 0;
    for (int i = 0; i < blocks; ++i) {
        const std::optional<Cdda::SubQ> q = Cdda::parseSubQ(Cdda::qFrameFromSubcode(QByteArrayView(raw).sliced(qsizetype(i) * SubcodeBytes, SubcodeBytes)));
        if (q && q->adr == 1) {
            ++valid;
            ++shifts[q->absoluteLba - (p + i)];
        }
    }
    if (valid == 0) {
        f.rwSubchannel = Feature::No;
        c.verdict = Feature::No;
        c.summary = Tr::tr("delivered, but unusable: no valid Q frame in it");
        return c;
    }
    int shift = shifts.cbegin().key();
    for (auto it = shifts.cbegin(); it != shifts.cend(); ++it)
        if (it.value() > shifts.value(shift))
            shift = it.key();
    f.rwSubchannel = Feature::Yes;
    f.rwShift = shift;
    c.verdict = Feature::Yes;
    c.details << Tr::tr("%1 of %2 blocks with a valid Q frame, the sub-channel is %3 sector(s) off").arg(valid).arg(blocks).arg(shift);

    const QByteArray rw = Cdda::rwFromSubcode(raw);
    const int asDelivered = Cdda::cdgGraphicsPacks(rw);
    const int deinterleaved = Cdda::cdgGraphicsPacks(Cdda::deinterleaveCdg(rw));
    if (asDelivered + deinterleaved == 0)
        c.details << Tr::tr("No CD+G graphics at this position of the disc.");
    else if (asDelivered > deinterleaved)
        c.details << Tr::tr("CD+G graphics on this disc, delivered already de-interleaved.");
    else
        c.details << Tr::tr("CD+G graphics on this disc, delivered as on the disc (de-interleaved by Audex).");
    c.summary =
        shift == 0 ? Tr::tr("delivered: CD+G graphics can be read") : Tr::tr("delivered, %1 sector(s) off (corrected): CD+G graphics can be read").arg(shift);
    return c;
}

}

// ---- the assistant ---------------------------------------------------------

DriveFeatures detectDriveFeatures(SectorReader &reader, const Cdda::Toc &toc, const FeatureCallbacks &callbacks)
{
    using Check = std::function<FeatureCheck(Context &, DriveFeatures &)>;
    struct Step {
        QString id;
        QString name;
        Check run;
    };
    // The cache has to be known before a read can be repeated reliably. The
    // checks that can hang a drive (or its USB bridge) until it is switched
    // off come last: reads outside the audio area, and C2 pointers, which some
    // bridges do not survive.
    QList<Step> steps{{u"burst"_s, Tr::tr("Read command size"), &checkBurst},
                      {u"cache"_s, Tr::tr("Audio cache"), &checkCache},
                      {u"stream"_s, Tr::tr("Accurate stream"), &checkStream},
                      {u"subchannel"_s, Tr::tr("Q sub-channel"), &checkSubchannel}};
    if (callbacks.offset)
        steps.append({u"offset"_s, Tr::tr("Read offset"), [&callbacks](Context &, DriveFeatures &) {
                          return callbacks.offset();
                      }});
    steps.append({u"rw"_s, Tr::tr("R-W sub-channel"), &checkRw});
    steps.append({u"overread"_s, Tr::tr("Overread"), &checkOverread});
    steps.append({u"c2"_s, Tr::tr("C2 error pointers"), &checkC2});

    DriveFeatures features;
    Context ctx{reader, ReadableArea{toc.audioStartLba(), toc.audioEndLba()}, callbacks.isCanceled, ProbeSectors, 4};
    QString lastTimeout; // the check that last ran into a timeout
    const auto noteStall = [&] {
        if (ctx.stalled && features.stalledAfter.isEmpty())
            features.stalledAfter = lastTimeout;
    };
    for (qsizetype i = 0; i < steps.size(); ++i) {
        if (ctx.canceled())
            break;
        FeatureCheck check;
        check.id = steps.at(i).id;
        check.name = steps.at(i).name;
        if (callbacks.started)
            callbacks.started(int(i), int(steps.size()), check);

        FeatureCheck result;
        if (ctx.answering()) {
            const int timeouts = reader.commandTimeouts();
            result = steps.at(i).run(ctx, features);
            if (ctx.timedOutSince(timeouts))
                lastTimeout = check.id;
        } else {
            result.summary = ctx.canceled() ? Tr::tr("not tested: stopped") : Tr::tr("not tested: the drive stopped answering after a timeout");
        }
        noteStall();
        check.verdict = result.verdict;
        check.summary = result.summary;
        check.details = result.details;
        features.checks.append(check);
        if (callbacks.finished)
            callbacks.finished(check);
    }
    // After a timeout in the last check: does the drive still answer? The
    // caller must not send it anything else (unlocking the tray...) if not.
    if (!ctx.canceled())
        ctx.answering();
    noteStall();
    reader.setC2Enabled(false);

    features.driveStalled = ctx.stalled;
    features.canceled = ctx.canceled();
    if (!features.canceled)
        features.measured = QDateTime::currentDateTime();
    return features;
}

}
