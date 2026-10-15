/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "metadata/gnudb.h"
#include "metadata/musicbrainz.h"
#include "online/chosencovers.h"
#include "online/responsecache.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

using namespace Audex;
using namespace Qt::StringLiterals;

namespace
{

// Three audio tracks; track 2 starts at the given LBA
CDInfo disc(int track2)
{
    Cdda::Toc toc;
    toc.tracks = {Cdda::Track{1, 1, 0, track2 - 1}, Cdda::Track{2, 1, track2, 9999}, Cdda::Track{3, 1, 10000, 14999}};
    toc.leadOutLba = 15000;
    return CDInfo(toc);
}

const QUrl url(u"https://musicbrainz.org/ws/2/discid/abc?toc=1+3+15150+150+5150+10150&fmt=json"_s);

}

AUDEX_TEST("response cache: discs with the same CDDB ID keep apart")
{
    // 5000 and 5010 fall into the same second: same CDDB ID, different TOC
    const CDInfo a = disc(5000);
    const CDInfo b = disc(5010);
    AUDEX_EQUAL(t, a.cddbDiscId(), b.cddbDiscId());
    AUDEX_CHECK(t, ResponseCache::key(Gnudb::queryUrl(a, u"hello"_s)) != ResponseCache::key(Gnudb::queryUrl(b, u"hello"_s)));
    AUDEX_CHECK(t, ResponseCache::key(MusicBrainz::discIdLookupUrl(a)) != ResponseCache::key(MusicBrainz::discIdLookupUrl(b)));
}

AUDEX_TEST("response cache: the gnudb email address is not part of the key")
{
    const CDInfo a = disc(5000);
    const QString one = ResponseCache::key(Gnudb::queryUrl(a, Gnudb::helloString(u"one@example.org"_s, u"Audex"_s, u"2.0"_s)));
    const QString two = ResponseCache::key(Gnudb::queryUrl(a, Gnudb::helloString(u"two@example.org"_s, u"Audex"_s, u"2.0"_s)));
    AUDEX_EQUAL(t, one, two);
}

AUDEX_TEST("response cache: kept answers are found")
{
    QTemporaryDir dir;
    ResponseCache cache(dir.path());
    AUDEX_CHECK(t, !cache.find(u"musicbrainz"_s, url).has_value());

    cache.insert(u"musicbrainz"_s, url, "{\"id\":\"abc\"}");
    const std::optional<QByteArray> found = cache.find(u"musicbrainz"_s, url);
    AUDEX_CHECK(t, found.has_value());
    AUDEX_EQUAL(t, found.value_or(QByteArray()), QByteArray("{\"id\":\"abc\"}"));
    AUDEX_CHECK(t, !cache.find(u"gnudb"_s, url).has_value()); // one folder per service
    AUDEX_EQUAL(t, cache.size(), qint64(12));

    cache.insert(u"musicbrainz"_s, url, QByteArray()); // nothing to keep
    AUDEX_CHECK(t, cache.find(u"musicbrainz"_s, url).has_value());

    cache.clear();
    AUDEX_CHECK(t, !cache.find(u"musicbrainz"_s, url).has_value());
    AUDEX_EQUAL(t, cache.size(), qint64(0));
}

AUDEX_TEST("response cache: old answers expire")
{
    QTemporaryDir dir;
    ResponseCache cache(dir.path());
    cache.setMaximumAge(30);
    cache.insert(u"gnudb"_s, url, "entry");

    const QString file = QDir(dir.path()).filePath(u"gnudb/"_s + ResponseCache::key(url));
    AUDEX_CHECK(t, QFile::exists(file));
    {
        QFile f(file);
        AUDEX_CHECK(t, f.open(QIODevice::ReadWrite));
        AUDEX_CHECK(t, f.setFileTime(QDateTime::currentDateTime().addDays(-31), QFileDevice::FileModificationTime));
    }
    AUDEX_CHECK(t, !cache.find(u"gnudb"_s, url).has_value());
    AUDEX_CHECK(t, !QFile::exists(file));

    cache.setMaximumAge(0); // turned off
    cache.insert(u"gnudb"_s, url, "entry");
    AUDEX_CHECK(t, !QFile::exists(file));
}

AUDEX_TEST("response cache: the oldest answers go first")
{
    QTemporaryDir dir;
    ResponseCache cache(dir.path());
    cache.setMaximumSize(250);

    const QUrl first(u"https://coverartarchive.org/release/1/front-500"_s);
    const QUrl second(u"https://coverartarchive.org/release/2/front-500"_s);
    cache.insert(u"coverart"_s, first, QByteArray(200, 'a'));
    {
        QFile f(QDir(dir.path()).filePath(u"coverart/"_s + ResponseCache::key(first)));
        AUDEX_CHECK(t, f.open(QIODevice::ReadWrite));
        AUDEX_CHECK(t, f.setFileTime(QDateTime::currentDateTime().addSecs(-60), QFileDevice::FileModificationTime));
    }
    cache.insert(u"coverart"_s, second, QByteArray(200, 'b'));

    AUDEX_CHECK(t, !cache.find(u"coverart"_s, first).has_value());
    AUDEX_CHECK(t, cache.find(u"coverart"_s, second).has_value());
    AUDEX_EQUAL(t, cache.size(), qint64(200));
}

AUDEX_TEST("response cache: without limits answers stay")
{
    QTemporaryDir dir;
    ResponseCache cache(dir.path());
    cache.setMaximumAge(ResponseCache::NoLimit);
    cache.setMaximumSize(ResponseCache::NoLimit);

    const QUrl first(u"https://coverartarchive.org/release/1/front-500"_s);
    const QUrl second(u"https://coverartarchive.org/release/2/front-500"_s);
    cache.insert(u"coverart"_s, first, QByteArray(200, 'a'));
    {
        QFile f(QDir(dir.path()).filePath(u"coverart/"_s + ResponseCache::key(first)));
        AUDEX_CHECK(t, f.open(QIODevice::ReadWrite));
        AUDEX_CHECK(t, f.setFileTime(QDateTime::currentDateTime().addDays(-3650), QFileDevice::FileModificationTime));
    }
    cache.insert(u"coverart"_s, second, QByteArray(200, 'b'));

    AUDEX_CHECK(t, cache.find(u"coverart"_s, first).has_value());
    AUDEX_CHECK(t, cache.find(u"coverart"_s, second).has_value());
    AUDEX_EQUAL(t, cache.size(), qint64(400));
}

AUDEX_TEST("response cache: the cover chosen for a disc is remembered and forgotten")
{
    QTemporaryDir dir;
    ResponseCache cache(dir.path());
    const QString discId = disc(5000).musicBrainzDiscId();
    const QString otherId = disc(5010).musicBrainzDiscId();
    AUDEX_CHECK(t, !ChosenCovers::find(cache, discId).has_value());

    Metadata::CoverArt cover;
    cover.data = QByteArray("\x89PNG\r\n\x1a\n", 8) + QByteArray(100, 'p');
    cover.mimeType = u"image/png"_s;
    cover.source = u"https://coverartarchive.org/release/1/2-500.png"_s;
    cover.origin = u"Cover Art Archive: back cover"_s;
    cover.originPage = QUrl(u"https://musicbrainz.org/release/1/cover-art"_s);
    ChosenCovers::keep(cache, discId, cover);

    const std::optional<Metadata::CoverArt> found = ChosenCovers::find(cache, discId);
    AUDEX_CHECK(t, found.has_value());
    if (found) {
        AUDEX_CHECK(t, *found == cover);
        AUDEX_EQUAL(t, found->source, cover.source);
        AUDEX_EQUAL(t, found->origin, cover.origin);
        AUDEX_EQUAL(t, found->originPage.toString(), cover.originPage.toString());
    }
    AUDEX_CHECK(t, !ChosenCovers::find(cache, otherId).has_value());

    ChosenCovers::forget(cache, discId);
    AUDEX_CHECK(t, !ChosenCovers::find(cache, discId).has_value());
    AUDEX_EQUAL(t, cache.size(), qint64(0));

    ChosenCovers::keep(cache, discId, cover);
    cache.setMaximumAge(0); // keeping turned off
    AUDEX_CHECK(t, !ChosenCovers::find(cache, discId).has_value());
}
