/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "utils/cuesheetwriter.h"

using namespace Audex;
using namespace Qt::StringLiterals;

namespace
{

// three tracks of 1000 sectors; track 1 starts at firstLba (> 0: hidden track one audio)
CDInfo disc(int firstLba = 0)
{
    Cdda::Toc toc;
    toc.tracks = {Cdda::Track{1, 1, firstLba, 999}, Cdda::Track{2, 1, 1000, 1999}, Cdda::Track{3, 1, 2000, 2999}};
    toc.leadOutLba = 3000;
    return CDInfo(toc);
}

CueSheetWriter writer(const CDInfo &info)
{
    Cdda::SubchannelScan scan;
    scan.index00 = {{2, 900}, {3, 1850}};
    scan.indexes = {{3, {2500}}};
    CueSheetWriter w(info);
    w.setSubchannel(scan);
    return w;
}

// only the lines that place the audio
QStringList layout(const QStringList &cue)
{
    QStringList result;
    for (const QString &line : cue) {
        const QString l = line.trimmed();
        if (l.startsWith(u"FILE"_s) || l.startsWith(u"TRACK"_s) || l.startsWith(u"INDEX"_s))
            result << l;
    }
    return result;
}

const QStringList Files{u"01.flac"_s, u"02.flac"_s, u"03.flac"_s};

}

AUDEX_TEST("cue sheet: track files with the pregaps appended to the previous track")
{
    const QStringList expected{u"FILE \"01.flac\" WAVE"_s,
                               u"TRACK 01 AUDIO"_s,
                               u"INDEX 01 00:00:00"_s,
                               u"TRACK 02 AUDIO"_s,
                               u"INDEX 00 00:12:00"_s,
                               u"FILE \"02.flac\" WAVE"_s,
                               u"INDEX 01 00:00:00"_s,
                               u"TRACK 03 AUDIO"_s,
                               u"INDEX 00 00:11:25"_s,
                               u"FILE \"03.flac\" WAVE"_s,
                               u"INDEX 01 00:00:00"_s,
                               u"INDEX 02 00:06:50"_s};
    AUDEX_EQUAL(t, layout(writer(disc()).cueSheet(Files, {1, 2, 3})).join(u'|'), expected.join(u'|'));
}

AUDEX_TEST("cue sheet: track files with the pregaps in their own track")
{
    const QStringList expected{u"FILE \"01.flac\" WAVE"_s,
                               u"TRACK 01 AUDIO"_s,
                               u"INDEX 01 00:00:00"_s,
                               u"FILE \"02.flac\" WAVE"_s,
                               u"TRACK 02 AUDIO"_s,
                               u"INDEX 00 00:00:00"_s,
                               u"INDEX 01 00:01:25"_s,
                               u"FILE \"03.flac\" WAVE"_s,
                               u"TRACK 03 AUDIO"_s,
                               u"INDEX 00 00:00:00"_s,
                               u"INDEX 01 00:02:00"_s,
                               u"INDEX 02 00:08:50"_s};
    const QStringList cue = writer(disc()).cueSheet(Files, {1, 2, 3}, false, false, CueSheetWriter::Pregaps::WithOwnTrack);
    AUDEX_EQUAL(t, layout(cue).join(u'|'), expected.join(u'|'));
}

AUDEX_TEST("cue sheet: a pregap behind a track that was not ripped")
{
    // appended: track 3's pregap is in the file of track 2, which is missing
    const QStringList appended = layout(writer(disc()).cueSheet({Files.at(0), Files.at(2)}, {1, 3}));
    const QStringList expectedAppended{u"FILE \"01.flac\" WAVE"_s,
                                       u"TRACK 01 AUDIO"_s,
                                       u"INDEX 01 00:00:00"_s,
                                       u"FILE \"03.flac\" WAVE"_s,
                                       u"TRACK 02 AUDIO"_s,
                                       u"INDEX 01 00:00:00"_s,
                                       u"INDEX 02 00:06:50"_s};
    AUDEX_EQUAL(t, appended.join(u'|'), expectedAppended.join(u'|'));

    // with their own track it is still there
    const QStringList own = layout(writer(disc()).cueSheet({Files.at(2)}, {3}, false, false, CueSheetWriter::Pregaps::WithOwnTrack));
    const QStringList expectedOwn{u"FILE \"03.flac\" WAVE"_s, u"TRACK 01 AUDIO"_s, u"INDEX 00 00:00:00"_s, u"INDEX 01 00:02:00"_s, u"INDEX 02 00:08:50"_s};
    AUDEX_EQUAL(t, own.join(u'|'), expectedOwn.join(u'|'));
}

AUDEX_TEST("cue sheet: hidden track one audio is the pregap of the first track")
{
    const QStringList files{u"00.flac"_s, u"01.flac"_s, u"02.flac"_s};
    const QStringList expected{u"FILE \"00.flac\" WAVE"_s, u"TRACK 01 AUDIO"_s, u"INDEX 00 00:00:00"_s, u"FILE \"01.flac\" WAVE"_s, u"INDEX 01 00:00:00"_s};
    for (const auto pregaps : {CueSheetWriter::Pregaps::AppendedToPrevious, CueSheetWriter::Pregaps::WithOwnTrack}) {
        const QStringList cue = layout(writer(disc(300)).cueSheet(files, {0, 1, 2}, false, false, pregaps));
        AUDEX_EQUAL(t, cue.mid(0, 5).join(u'|'), expected.join(u'|'));
        AUDEX_CHECK(t, cue.contains(u"TRACK 02 AUDIO"_s) && !cue.contains(u"TRACK 03 AUDIO"_s));
    }
}
