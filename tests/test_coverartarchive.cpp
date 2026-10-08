/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "online/coverartarchive.h"

using namespace Audex;

namespace
{

// shortened answer of https://coverartarchive.org/release/<mbid>
const char Listing[] = R"({
  "images": [
    {
      "approved": true, "back": false, "comment": "", "edit": 1, "front": true, "id": 1,
      "image": "http://coverartarchive.org/release/abc/1.jpg",
      "thumbnails": {
        "250": "http://coverartarchive.org/release/abc/1-250.jpg",
        "500": "http://coverartarchive.org/release/abc/1-500.jpg",
        "1200": "http://coverartarchive.org/release/abc/1-1200.jpg",
        "large": "http://coverartarchive.org/release/abc/1-500.jpg",
        "small": "http://coverartarchive.org/release/abc/1-250.jpg"
      },
      "types": ["Front"]
    },
    {
      "approved": true, "back": false, "comment": "inner sleeve", "edit": 2, "front": false, "id": 2,
      "image": "https://ia800.us.archive.org/abc/2.png",
      "thumbnails": {
        "large": "http://coverartarchive.org/release/abc/2-500.jpg",
        "small": "http://coverartarchive.org/release/abc/2-250.jpg"
      },
      "types": ["Booklet", "Liner"]
    }
  ],
  "release": "http://musicbrainz.org/release/abc"
})";

}

AUDEX_TEST("Cover Art Archive listing: images, thumbnails and types")
{
    QString error;
    const std::optional<CoverArtListing> listing = parseCoverArtListing(QByteArray(Listing), &error);
    AUDEX_CHECK_MSG(t, listing.has_value(), error);
    if (!listing)
        return;
    AUDEX_EQUAL(t, listing->images.size(), qsizetype(2));
    AUDEX_CHECK(t, listing->release == QUrl(QStringLiteral("https://musicbrainz.org/release/abc")));

    const CoverArtImage &front = listing->images.at(0);
    AUDEX_CHECK(t, front.front);
    AUDEX_CHECK(t, front.types == QStringList{QStringLiteral("Front")});
    // http addresses of the archive are switched to https
    AUDEX_CHECK(t, front.url(1200) == QUrl(QStringLiteral("https://coverartarchive.org/release/abc/1-1200.jpg")));
    AUDEX_CHECK(t, front.url(0) == QUrl(QStringLiteral("https://coverartarchive.org/release/abc/1.jpg")));

    // old style thumbnail names; no 1200 pixel thumbnail: the original
    const CoverArtImage &booklet = listing->images.at(1);
    AUDEX_CHECK(t, booklet.comment == QStringLiteral("inner sleeve"));
    AUDEX_CHECK(t, booklet.url(250) == QUrl(QStringLiteral("https://coverartarchive.org/release/abc/2-250.jpg")));
    AUDEX_CHECK(t, booklet.url(500) == QUrl(QStringLiteral("https://coverartarchive.org/release/abc/2-500.jpg")));
    AUDEX_CHECK(t, booklet.url(1200) == QUrl(QStringLiteral("https://ia800.us.archive.org/abc/2.png")));
}

AUDEX_TEST("Cover Art Archive listing: other data is rejected")
{
    AUDEX_CHECK(t, !parseCoverArtListing(QByteArray("{\"error\": \"not found\"}")).has_value());
    AUDEX_CHECK(t, !parseCoverArtListing(QByteArray("<html></html>")).has_value());
    const auto empty = parseCoverArtListing(QByteArray("{\"images\": []}"));
    AUDEX_CHECK(t, empty.has_value() && empty->images.isEmpty());
}
