/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "metadata/cdinfo.h"

using namespace Audex;
using namespace Audex::Metadata;
using namespace Qt::StringLiterals;

namespace
{

// hidden track one audio and two tracks
CDInfo disc()
{
    Cdda::Toc toc;
    toc.tracks = {Cdda::Track{1, 1, 300, 999}, Cdda::Track{2, 1, 1000, 1999}};
    toc.leadOutLba = 2000;
    return CDInfo(toc);
}

}

AUDEX_TEST("cd info: clearing the metadata keeps what belongs to the disc")
{
    CDInfo info = disc();
    info.metadata().setText(Field::MCN, u"4000000000006"_s);
    info.trackMetadata(1).setText(Field::ISRC, u"DEA000000001"_s);
    info.completeMetadata();
    info.metadata().confirm();

    MetadataCandidate candidate;
    candidate.album.setText(Field::Artist, u"The Test Tones"_s);
    candidate.album.setText(Field::Album, u"Sine Waves"_s);
    candidate.album.setFlag(Field::VariousArtists, true);
    candidate.album.track(0).setText(Field::Artist, u"Somebody Else"_s);
    candidate.album.track(1).setText(Field::Title, u"440 Hz"_s);
    candidate.album.track(2).setText(Field::Title, u"880 Hz"_s);
    const CoverArt cover{QByteArray("cover"), u"image/jpeg"_s, QString(), QString(), QUrl()};
    candidate.album.setCover(cover);
    info.applyCandidate(candidate);
    AUDEX_EQUAL(t, info.metadata().text(Field::Artist), u"The Test Tones"_s);
    AUDEX_EQUAL(t, info.trackMetadata(0).text(Field::Artist), u"Somebody Else"_s);
    AUDEX_CHECK(t, !info.metadata().cover().isNull());

    info.clearMetadata();
    const Album &album = info.metadata();
    AUDEX_CHECK(t, !album.contains(Field::Artist));
    AUDEX_CHECK(t, !album.contains(Field::Album));
    AUDEX_CHECK(t, !album.flag(Field::VariousArtists));
    AUDEX_CHECK(t, !album.track(0).contains(Field::Artist));
    AUDEX_CHECK(t, !album.track(1).contains(Field::Title));
    AUDEX_CHECK(t, !album.track(2).contains(Field::Title));
    AUDEX_CHECK(t, album.cover().isNull());
    AUDEX_EQUAL(t, album.text(Field::MCN), u"4000000000006"_s);
    AUDEX_EQUAL(t, album.track(1).text(Field::ISRC), u"DEA000000001"_s);
    AUDEX_EQUAL(t, album.text(Field::CddbDiscId), info.cddbDiscId());
    AUDEX_EQUAL(t, album.text(Field::MusicBrainzDiscId), info.musicBrainzDiscId());
    AUDEX_CHECK(t, album.isModified()); // the editor sees the removals
}
