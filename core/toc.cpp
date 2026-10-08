/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "toc.h"

using namespace Qt::StringLiterals;

namespace Audex::Cdda
{

QString sectorsToMsf(qint64 sectors)
{
    const bool negative = sectors < 0;
    qint64 s = negative ? -sectors : sectors;
    const qint64 frames = s % SectorsPerSecond;
    s /= SectorsPerSecond;
    return u"%1%2:%3.%4"_s.arg(negative ? u"-"_s : QString())
        .arg(s / 60, 2, 10, QLatin1Char('0'))
        .arg(s % 60, 2, 10, QLatin1Char('0'))
        .arg(frames, 2, 10, QLatin1Char('0'));
}

QString sectorsToTime(qint64 sectors)
{
    const bool negative = sectors < 0;
    qint64 s = negative ? -sectors : sectors;
    const qint64 ms = (s % SectorsPerSecond) * 1000 / SectorsPerSecond;
    s /= SectorsPerSecond;
    return u"%1%2:%3.%4"_s.arg(negative ? u"-"_s : QString())
        .arg(s / 60)
        .arg(s % 60, 2, 10, QLatin1Char('0'))
        .arg(ms, 3, 10, QLatin1Char('0'));
}

bool Toc::isValid(QString *error) const
{
    auto fail = [error](const QString &msg) {
        if (error)
            *error = msg;
        return false;
    };

    if (tracks.isEmpty())
        return fail(u"TOC contains no tracks"_s);

    for (int i = 0; i < tracks.size(); ++i) {
        const Track &t = tracks.at(i);
        if (t.number < 1 || t.number > 99)
            return fail(u"Invalid track number %1"_s.arg(t.number));
        if (t.lastLba < t.firstLba)
            return fail(u"Track %1 has an invalid range (%2..%3)"_s.arg(t.number).arg(t.firstLba).arg(t.lastLba));
        if (t.firstLba < 0)
            return fail(u"Track %1 starts before LBA 0"_s.arg(t.number));
        if (i > 0) {
            const Track &p = tracks.at(i - 1);
            if (t.number <= p.number)
                return fail(u"Tracks are not sorted"_s);
            if (t.firstLba <= p.lastLba)
                return fail(u"Tracks %1 and %2 overlap"_s.arg(p.number).arg(t.number));
        }
    }

    if (leadOutLba <= tracks.last().lastLba)
        return fail(u"Lead-out (%1) is not behind the last track"_s.arg(leadOutLba));

    return true;
}

const Track *Toc::track(int number) const
{
    for (const Track &t : tracks)
        if (t.number == number)
            return &t;
    return nullptr;
}

QList<int> Toc::audioTrackNumbers() const
{
    QList<int> result;
    for (const Track &t : tracks)
        if (t.audio)
            result.append(t.number);
    return result;
}

int Toc::firstAudioTrackNumber() const
{
    for (const Track &t : tracks)
        if (t.audio)
            return t.number;
    return 0;
}

int Toc::lastAudioTrackNumber() const
{
    for (auto it = tracks.crbegin(); it != tracks.crend(); ++it)
        if (it->audio)
            return it->number;
    return 0;
}

int Toc::audioStartLba() const
{
    // HTOA belongs to the audio area if track 1 is an audio track.
    const Track *first = track(firstAudioTrackNumber());
    if (!first)
        return 0;
    if (first->number == tracks.first().number)
        return 0;
    return first->firstLba;
}

int Toc::audioEndLba() const
{
    const Track *last = track(lastAudioTrackNumber());
    return last ? last->lastLba + 1 : 0;
}

int Toc::htoaSectorCount() const
{
    if (tracks.isEmpty())
        return 0;
    const Track &first = tracks.first();
    if (first.number != 1 || !first.audio)
        return 0;
    return first.firstLba;
}

QStringList Toc::describe() const
{
    QStringList lines;
    lines << u"     Track |   Start  |  Length  | Start sector | End sector | Type"_s;
    lines << u"    -------+----------+----------+--------------+------------+----------------"_s;
    for (const Track &t : tracks) {
        QString type = t.audio ? u"audio"_s : u"data"_s;
        if (t.session > 1)
            type += u", session %1"_s.arg(t.session);
        if (t.preEmphasis)
            type += u", pre-emphasis"_s;
        if (t.fourChannel)
            type += u", 4 channels"_s;
        lines << u"       %1  | %2 | %3 |    %4    |   %5   | %6"_s.arg(t.number, 2)
                     .arg(sectorsToMsf(t.firstLba))
                     .arg(sectorsToMsf(t.sectorCount()))
                     .arg(t.firstLba, 6)
                     .arg(t.lastLba, 6)
                     .arg(type);
    }
    lines << u"    Lead-out: %1 (%2)"_s.arg(leadOutLba).arg(sectorsToMsf(leadOutLba));
    if (htoaSectorCount() > 0)
        lines << u"    Hidden track one audio: %1 sectors (%2)"_s.arg(htoaSectorCount()).arg(sectorsToMsf(htoaSectorCount()));
    if (!source.isEmpty())
        lines << u"    Source: %1"_s.arg(source);
    return lines;
}

}
