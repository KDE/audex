/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "encoding/tagwriter.h"
#include "metadata/cdinfo.h"

using namespace Audex;
using namespace Qt::StringLiterals;

AUDEX_TEST("pre-emphasis: flagged audio tracks and the tag")
{
    Cdda::Toc toc;
    toc.tracks = {Cdda::Track{1, 1, 0, 999}, Cdda::Track{2, 1, 1000, 1999}, Cdda::Track{3, 1, 2000, 2999}, Cdda::Track{4, 1, 3000, 3999}};
    toc.tracks[1].preEmphasis = true;
    toc.tracks[3].preEmphasis = true;
    toc.tracks[3].audio = false; // the flag means nothing for a data track
    toc.leadOutLba = 4000;
    const CDInfo info(toc);
    AUDEX_CHECK(t, info.preEmphasisTracks() == QList<int>{2});

    Encoding::TagInfo tags;
    tags.trackNumber = 2;
    tags.displayTrackNumber = 2;
    tags.extra.insert(u" pre_emphasis "_s, u"1"_s);
    tags.extra.insert(u"  "_s, u"ignored"_s);
    const QMap<QString, QStringList> properties = Encoding::TagWriter::properties(tags);
    AUDEX_CHECK(t, properties.value(u"PRE_EMPHASIS"_s) == QStringList{u"1"_s});
    AUDEX_CHECK(t, !properties.contains(QString()) && !properties.values().contains(QStringList{u"ignored"_s}));
}
