/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ripreport.h"

#include <QDateTime>

using namespace Qt::StringLiterals;

namespace Audex::Rip
{

namespace
{
QString hex8(quint32 v)
{
    return u"%1"_s.arg(v, 8, 16, QLatin1Char('0')).toUpper();
}
QString yesNo(bool b)
{
    return b ? u"Yes"_s : u"No"_s;
}
}

QString modeDescription(const Rip::RipOptions &o)
{
    if (o.mode == Rip::ReadMode::Fast)
        return u"Fast (single read, %1 retries on read errors)"_s.arg(o.readErrorRetries);
    const QString first = o.verifyFirst ? u", AccurateRip first"_s : QString();
    const QString retries = u", %1 retries on read errors"_s.arg(o.readErrorRetries);
    if (o.useC2)
        return u"Secure with C2%1 (window %2, %3 reads/round, %4 rounds%5)"_s.arg(first).arg(o.secureWindow).arg(o.readsPerRound).arg(o.maxRounds).arg(retries);
    return u"Secure%1 (window %2, %3 of %4 identical reads required, %5 rounds%6)"_s.arg(first)
        .arg(o.secureWindow)
        .arg(o.requiredMatches)
        .arg(o.readsPerRound)
        .arg(o.maxRounds)
        .arg(retries);
}

QStringList formatReport(const ReportContext &ctx, const Rip::RipResult &result)
{
    const Rip::RipOptions &o = ctx.options;
    QStringList l;

    l << u"%1 extraction log from %2"_s.arg(ctx.application, QDateTime::currentDateTime().toString(Qt::ISODate));
    l << QString();
    l << u"Used drive  : %1"_s.arg(ctx.drive);
    l << u"Device      : %1"_s.arg(ctx.device);
    if (!ctx.medium.isEmpty())
        l << u"Medium      : %1"_s.arg(ctx.medium);
    l << QString();
    l << u"Read mode               : %1"_s.arg(modeDescription(o));
    l << u"Read offset correction  : %1"_s.arg(o.readOffset);
    if (!ctx.gapHandling.isEmpty())
        l << u"Gap handling            : %1"_s.arg(ctx.gapHandling);
    l << u"Overread lead-in/out    : %1"_s.arg(yesNo(o.overread));
    l << u"Use C2 error pointers   : %1"_s.arg(yesNo(o.useC2));
    l << u"Defeat audio cache      : %1"_s.arg(o.cacheDefeat ? u"Yes (distance %1 sectors, %2 far read(s))"_s.arg(o.cacheDefeatDistance).arg(o.cacheDefeatReads)
                                                             : u"No"_s);
    l << u"Read speed              : %1"_s.arg(o.readSpeed > 0 ? u"%1x"_s.arg(o.readSpeed) : u"maximum"_s);
    l << u"Speed during recovery   : %1"_s.arg(o.errorReadSpeed > 0 ? u"%1x"_s.arg(o.errorReadSpeed) : u"unchanged"_s);
    for (int shift : o.accurateRipShifts)
        l << u"Other pressing checked  : offset %1 samples (AccurateRip)"_s.arg(shift > 0 ? u"+%1"_s.arg(shift) : QString::number(shift));
    if (!ctx.encoder.isEmpty())
        l << u"Output format           : %1"_s.arg(ctx.encoder);
    for (const QString &n : ctx.driveNotes)
        l << u"Note                    : %1"_s.arg(n);
    l << QString();
    l << u"TOC of the extracted CD"_s;
    l << QString();
    l << ctx.toc.describe();
    l << QString();
    if (!ctx.subchannel.isEmpty()) {
        l << ctx.subchannel;
        l << QString();
    }

    if (ctx.repaired) {
        l << u"Checksums of the image after its repair with the CUETools database"_s;
        l << QString();
    }

    for (int i = 0; i < result.segments.size(); ++i) {
        const Rip::SegmentResult &s = result.segments.at(i);
        const Rip::Segment &seg = s.segment;
        const int sectors = seg.lastLba - seg.firstLba + 1;

        l << (seg.trackNumber == 0 ? u"Hidden track one audio"_s : u"Track %1"_s.arg(seg.trackNumber, 2, 10, QLatin1Char('0')));
        l << QString();
        l << u"     Filename %1"_s.arg(ctx.fileNames.value(i));
        l << u"     Range LBA %1 - %2 (%3)"_s.arg(seg.firstLba).arg(seg.lastLba).arg(Cdda::sectorsToMsf(sectors));
        if (seg.firstLba < seg.checksumFirst())
            l << u"     Pregap LBA %1 - %2 (%3) at the start of the file"_s.arg(seg.firstLba)
                     .arg(seg.checksumFirst() - 1)
                     .arg(Cdda::sectorsToMsf(seg.checksumFirst() - seg.firstLba));
        l << u"     Peak level %1 %"_s.arg(s.peakPercent, 0, 'f', 1);
        if (s.paddedSectors > 0)
            l << u"     Sectors filled with silence (not overread) %1"_s.arg(s.paddedSectors);
        if (s.rereadSectors > 0)
            l << u"     Sectors needing error recovery %1"_s.arg(s.rereadSectors);
        for (const Rip::SuspiciousPosition &p : s.suspicious) {
            l << u"     Suspicious position %1%2 (%3)"_s.arg(Cdda::sectorsToTime(p.lba - seg.firstLba))
                     .arg(p.zeroFilled ? u", filled with silence"_s : QString())
                     .arg(p.reason);
        }
        l << u"     Copy CRC %1 (w/o null samples %2)"_s.arg(hex8(s.crc32), hex8(s.crc32NoNull));
        if (seg.accurateRip) {
            l << u"     AccurateRip v1 %1, v2 %2, frame 450 %3"_s.arg(hex8(s.accurateRipV1), hex8(s.accurateRipV2), hex8(s.accurateRipFrame450));
            for (const Rip::ShiftedChecksums &c : s.accurateRipShifted)
                l << u"     AccurateRip at offset %1: v1 %2, v2 %3"_s.arg(c.shift > 0 ? u"+%1"_s.arg(c.shift) : QString::number(c.shift),
                                                                          hex8(c.v1),
                                                                          hex8(c.v2));
        }
        for (const QString &note : ctx.trackNotes.value(i))
            l << u"     "_s + note;
        if (ctx.crcBeforeRepair.contains(i))
            l << u"     Repaired with the CUETools database (copy CRC before %1)"_s.arg(hex8(ctx.crcBeforeRepair.value(i)));
        if (s.acceptedAfterOnePass)
            l << u"     Read once, confirmed by AccurateRip"_s;
        if (s.bytesWritten != seg.byteCount())
            l << u"     Incomplete: %1 of %2 bytes"_s.arg(s.bytesWritten).arg(seg.byteCount());
        else if (s.suspicious.isEmpty())
            l << u"     Copy OK"_s;
        else
            l << u"     Copy finished with %1 suspicious position(s)"_s.arg(s.suspicious.size());
        l << QString();
    }

    if (!ctx.accurateRip.isEmpty()) {
        l << ctx.accurateRip;
        l << QString();
    }
    if (!ctx.ctdb.isEmpty()) {
        l << ctx.ctdb;
        l << QString();
    }

    const Rip::RipStatistics &st = result.statistics;
    l << u"Statistics"_s;
    l << u"     Read commands %1, sectors requested %2, cache defeats %3"_s.arg(st.readCommands).arg(st.sectorsRequested).arg(st.cacheDefeats);
    l << u"     Pass mismatches %1, recovered by drive %2"_s.arg(st.passMismatches).arg(st.recoveredByDrive);
    l << u"     Duration %1"_s.arg(QTime(0, 0).addMSecs(int(st.elapsedMs)).toString(u"hh:mm:ss"_s));
    l << QString();

    for (const QString &n : ctx.outputNotes)
        l << u"Output warning: %1"_s.arg(n);
    if (!ctx.outputNotes.isEmpty())
        l << QString();

    if (result.canceled)
        l << u"Extraction was canceled."_s;
    else if (!result.completed)
        l << u"Extraction failed: %1"_s.arg(result.error);
    else if (result.hasSuspiciousPositions() && ctx.repaired)
        l << u"There were errors; the image was repaired with the CUETools database."_s;
    else if (result.hasSuspiciousPositions())
        l << u"There were errors."_s;
    else
        l << u"No errors occurred."_s;
    l << u"End of status report"_s;
    return l;
}

}
