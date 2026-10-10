/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "metadata/gnudb.h"

using namespace Audex;
using namespace Qt::StringLiterals;

namespace
{

using Metadata::Field;

// Three audio tracks, lead-out at LBA 15000
CDInfo disc3()
{
    Cdda::Toc toc;
    toc.tracks = {Cdda::Track{1, 1, 0, 4999}, Cdda::Track{2, 1, 5000, 9999}, Cdda::Track{3, 1, 10000, 14999}};
    toc.leadOutLba = 15000;
    return CDInfo(toc);
}

// Mixed Mode: data track 1, two audio tracks
CDInfo discMixed()
{
    Cdda::Toc toc;
    toc.tracks = {Cdda::Track{1, 1, 0, 4999, false}, Cdda::Track{2, 1, 5000, 9999}, Cdda::Track{3, 1, 10000, 14999}};
    toc.leadOutLba = 15000;
    return CDInfo(toc);
}

QByteArray entry3(const char *dtitle, const char *t0, const char *t1, const char *t2)
{
    return QByteArray("210 rock 12345678 CD database entry follows (until terminating `.')\r\n"
                      "# xmcd\r\n"
                      "#\r\n"
                      "# Track frame offsets:\r\n"
                      "#\t150\r\n"
                      "#\t5150\r\n"
                      "#\t10150\r\n"
                      "#\r\n"
                      "# Disc length: 202 seconds\r\n"
                      "#\r\n"
                      "# Revision: 3\r\n"
                      "# Submitted via: CDex 1.51\r\n"
                      "#\r\n"
                      "DISCID=03015e03\r\n")
        + "DTITLE=" + dtitle + "\r\n" //
        + "DYEAR=1981\r\nDGENRE=New Age\r\n" //
        + "TTITLE0=" + t0 + "\r\n" //
        + "TTITLE1=" + t1 + "\r\n" //
        + "TTITLE2=" + t2 + "\r\n" //
        + "EXTD=\r\nEXTT0=\r\nEXTT1=\r\nEXTT2=\r\nPLAYORDER=\r\n.\r\n";
}

}

AUDEX_TEST("gnudb: hello string and email check")
{
    AUDEX_CHECK(t, Gnudb::isUsableEmail(u"marco@example.org"_s));
    AUDEX_CHECK(t, !Gnudb::isUsableEmail(u"marco"_s));
    AUDEX_CHECK(t, !Gnudb::isUsableEmail(u"marco@localhost"_s)); // no domain
    AUDEX_CHECK(t, !Gnudb::isUsableEmail(u"@example.org"_s));
    AUDEX_CHECK(t, !Gnudb::isUsableEmail(QString()));

    AUDEX_CHECK(t, Gnudb::helloString(u"marco@example.org"_s, u"Audex"_s, u"2.0.0"_s) == u"marco example.org Audex 2.0.0"_s);
    // without a usable address there is no hello and therefore no request
    AUDEX_CHECK(t, Gnudb::helloString(u"nonsense"_s, u"Audex"_s, u"2.0.0"_s).isEmpty());
    // spaces in the fields would break the command
    AUDEX_CHECK(t, Gnudb::helloString(u"marco@example.org"_s, u"My App"_s, u"1.0"_s) == u"marco example.org MyApp 1.0"_s);
}

AUDEX_TEST("gnudb: query command and url")
{
    const CDInfo disc = disc3();
    const QString command = Gnudb::queryCommand(disc);
    // offsets are absolute (LBA + 150), the length is the lead-out in seconds
    AUDEX_CHECK_MSG(t, command == u"cddb query %1 3 150 5150 10150 202"_s.arg(disc.cddbDiscId()), command);

    const QUrl url = Gnudb::queryUrl(disc, u"marco example.org Audex 2.0.0"_s);
    const QString text = url.toString();
    AUDEX_CHECK_MSG(t, text.startsWith(u"https://gnudb.gnudb.org/~cddb/cddb.cgi?"_s), text);
    AUDEX_CHECK_MSG(t, text.contains(u"cmd=cddb+query+%1+3+150+5150+10150+202"_s.arg(disc.cddbDiscId())), text);
    AUDEX_CHECK_MSG(t, text.contains(u"hello=marco+example.org+Audex+2.0.0"_s), text);
    AUDEX_CHECK_MSG(t, text.contains(u"proto=6"_s), text);

    const QUrl read = Gnudb::readUrl(u"rock"_s, u"12345678"_s, u"marco example.org Audex 2.0.0"_s);
    AUDEX_CHECK_MSG(t, read.toString().contains(u"cmd=cddb+read+rock+12345678"_s), read.toString());

    // no TOC, no request
    AUDEX_CHECK(t, Gnudb::queryCommand(CDInfo()).isEmpty());
    AUDEX_CHECK(t, !Gnudb::queryUrl(CDInfo(), u"a b c d"_s).isValid());
    AUDEX_CHECK(t, !Gnudb::queryUrl(disc, QString()).isValid()); // no hello
}

AUDEX_TEST("gnudb: query answers")
{
    // one exact match: the entry stands on the status line
    const Gnudb::QueryResult one = Gnudb::parseQueryResponse("200 rock 12345678 Pink Floyd / The Wall\r\n");
    AUDEX_EQUAL(t, one.matches.size(), qsizetype(1));
    AUDEX_CHECK(t, one.error.isEmpty());
    if (!one.matches.isEmpty()) {
        AUDEX_CHECK(t, one.matches.at(0).category == u"rock"_s);
        AUDEX_CHECK(t, one.matches.at(0).discId == u"12345678"_s);
        AUDEX_CHECK(t, one.matches.at(0).title == u"Pink Floyd / The Wall"_s);
        AUDEX_CHECK(t, one.matches.at(0).exact);
    }

    const Gnudb::QueryResult many = Gnudb::parseQueryResponse("210 Found exact matches, list follows (until terminating `.')\r\n"
                                                              "rock 12345678 Pink Floyd / The Wall\r\n"
                                                              "misc 87654321 Pink Floyd / The Wall (Box)\r\n"
                                                              ".\r\n");
    AUDEX_EQUAL(t, many.matches.size(), qsizetype(2));
    if (many.matches.size() == 2) {
        AUDEX_CHECK(t, many.matches.at(1).category == u"misc"_s);
        AUDEX_CHECK(t, many.matches.at(0).exact && many.matches.at(1).exact);
    }

    const Gnudb::QueryResult inexact = Gnudb::parseQueryResponse("211 Found inexact matches, list follows (until terminating `.')\r\n"
                                                                 "folk 2407ef04 Bob Dylan / New York\r\n"
                                                                 ".\r\n");
    AUDEX_EQUAL(t, inexact.matches.size(), qsizetype(1));
    if (!inexact.matches.isEmpty())
        AUDEX_CHECK(t, !inexact.matches.at(0).exact);

    // nothing found is not an error
    const Gnudb::QueryResult none = Gnudb::parseQueryResponse("202 No match found\r\n");
    AUDEX_CHECK(t, none.matches.isEmpty() && none.error.isEmpty());

    // a server error is one
    AUDEX_CHECK(t, !Gnudb::parseQueryResponse("500 Internal server error\r\n").error.isEmpty());
    // an HTML error page is not a CDDB answer
    AUDEX_CHECK(t, !Gnudb::parseQueryResponse("<html><body>nope</body></html>").error.isEmpty());
}

AUDEX_TEST("gnudb: read answer becomes a candidate")
{
    const CDInfo disc = disc3();
    const Gnudb::ReadResult result = Gnudb::parseReadResponse(
        entry3("Con Spirito / Franske Stemninger", "Mille regretz", "L'arche", "Yez"), disc, u"rock"_s, u"12345678"_s, true);
    AUDEX_CHECK_MSG(t, result.found, result.error);
    if (!result.found)
        return;

    const MetadataCandidate &c = result.candidate;
    AUDEX_CHECK(t, c.provider == u"gnudb"_s);
    AUDEX_CHECK(t, c.reference == u"rock/12345678"_s);
    AUDEX_EQUAL(t, c.score, 80); // exact match, below MusicBrainz

    const Metadata::Album &a = c.album;
    AUDEX_CHECK(t, a.text(Field::Artist) == u"Con Spirito"_s);
    AUDEX_CHECK(t, a.text(Field::Album) == u"Franske Stemninger"_s);
    AUDEX_CHECK(t, a.text(Field::Year) == u"1981"_s);
    AUDEX_CHECK(t, a.text(Field::Genre) == u"New Age"_s);
    AUDEX_CHECK(t, a.text(Field::CddbCategory) == u"rock"_s);
    AUDEX_EQUAL(t, a.trackNumbers().size(), qsizetype(3));
    AUDEX_CHECK(t, a.track(1).text(Field::Title) == u"Mille regretz"_s);
    AUDEX_CHECK(t, a.track(3).text(Field::Title) == u"Yez"_s);
    // not a sampler: the track artists stay empty
    AUDEX_CHECK(t, a.track(1).text(Field::Artist).isEmpty());
    AUDEX_CHECK(t, !a.flag(Field::VariousArtists));

    // an inexact match scores lower
    const Gnudb::ReadResult inexact = Gnudb::parseReadResponse(entry3("A / B", "1", "2", "3"), disc, u"rock"_s, u"12345678"_s, false);
    AUDEX_CHECK(t, inexact.found);
    AUDEX_EQUAL(t, inexact.candidate.score, 50);

    // 401: the entry is gone, which is not an error
    const Gnudb::ReadResult gone = Gnudb::parseReadResponse("401 rock 12345678 No such CD entry in database\r\n", disc, u"rock"_s, u"12345678"_s, true);
    AUDEX_CHECK(t, !gone.found && gone.error.isEmpty());
}

AUDEX_TEST("gnudb: disc title without a separator, sampler with track artists")
{
    const CDInfo disc = disc3();

    // "If the '/' is absent, artist and disc title are the same" (xmcd format)
    const Gnudb::ReadResult same = Gnudb::parseReadResponse(entry3("Mogwai", "a", "b", "c"), disc, u"rock"_s, u"1"_s, true);
    AUDEX_CHECK(t, same.found);
    AUDEX_CHECK(t, same.candidate.album.text(Field::Artist) == u"Mogwai"_s);
    AUDEX_CHECK(t, same.candidate.album.text(Field::Album) == u"Mogwai"_s);

    // on a sampler the track artist stands in front of the title
    const Gnudb::ReadResult various = Gnudb::parseReadResponse(
        entry3("Various / Now 42", "Blur / Song 2", "Oasis / Wonderwall", "no artist here"), disc, u"misc"_s, u"2"_s, true);
    AUDEX_CHECK(t, various.found);
    const Metadata::Album &a = various.candidate.album;
    AUDEX_CHECK(t, a.flag(Field::VariousArtists));
    AUDEX_CHECK(t, a.track(1).text(Field::Artist) == u"Blur"_s);
    AUDEX_CHECK(t, a.track(1).text(Field::Title) == u"Song 2"_s);
    AUDEX_CHECK(t, a.track(2).text(Field::Artist) == u"Oasis"_s);
    // without a separator the whole text stays the title
    AUDEX_CHECK(t, a.track(3).text(Field::Artist).isEmpty());
    AUDEX_CHECK(t, a.track(3).text(Field::Title) == u"no artist here"_s);
}

AUDEX_TEST("gnudb: escapes, concatenated keywords and utf-8")
{
    AUDEX_CHECK(t, Gnudb::unescape(u"a\\nb"_s) == u"a\nb"_s);
    AUDEX_CHECK(t, Gnudb::unescape(u"a\\tb"_s) == u"a\tb"_s);
    AUDEX_CHECK(t, Gnudb::unescape(u"a\\\\b"_s) == u"a\\b"_s);
    AUDEX_CHECK(t, Gnudb::unescape(u"trailing\\"_s) == u"trailing\\"_s);

    const QByteArray raw = QByteArray("210 misc abcdef12 CD database entry follows (until terminating `.')\n"
                                      "# xmcd\n"
                                      "# Track frame offsets:\n"
                                      "#\t150\n"
                                      "#\t5150\n"
                                      "#\t10150\n"
                                      "#\n"
                                      "DTITLE=Einstürzende Neubauten / Zeichnungen des Patienten O. T.\n"
                                      "DYEAR=\n"
                                      "DGENRE=Rock\n"
                                      "TTITLE0=Vanadium-I-Ching\n"
                                      "TTITLE1=Zeichnungen des\n"
                                      "TTITLE1= Patienten O. T.\n"
                                      "TTITLE2=Armenia\n"
                                      "EXTD=Copyright (c) 1983\\nSome Bizzare\n"
                                      "PLAYORDER=\n"
                                      ".\n");
    const Gnudb::ReadResult result = Gnudb::parseReadResponse(raw, disc3(), u"misc"_s, u"abcdef12"_s, true);
    AUDEX_CHECK_MSG(t, result.found, result.error);
    if (!result.found)
        return;
    const Metadata::Album &a = result.candidate.album;
    AUDEX_CHECK(t, a.text(Field::Artist) == QString::fromUtf8("Einstürzende Neubauten"));
    // repeated keywords are concatenated without a separator
    AUDEX_CHECK_MSG(t, a.track(2).text(Field::Title) == u"Zeichnungen des Patienten O. T."_s, a.track(2).text(Field::Title));
    AUDEX_CHECK(t, a.text(Field::Comment) == u"Copyright (c) 1983\nSome Bizzare"_s);
    AUDEX_CHECK(t, a.text(Field::Year).isEmpty()); // empty means unknown, not 0
}

AUDEX_TEST("gnudb: entries for another disc are rejected")
{
    const CDInfo disc = disc3();

    // four titles, three tracks: a disc id collision
    const QByteArray raw = QByteArray("210 rock 12345678 CD database entry follows (until terminating `.')\n"
                                      "# xmcd\n"
                                      "# Track frame offsets:\n"
                                      "#\t150\n"
                                      "#\t5150\n"
                                      "#\t10150\n"
                                      "#\t12150\n"
                                      "#\n"
                                      "DTITLE=A / B\n"
                                      "TTITLE0=1\nTTITLE1=2\nTTITLE2=3\nTTITLE3=4\n"
                                      ".\n");
    const Gnudb::ReadResult wrong = Gnudb::parseReadResponse(raw, disc, u"rock"_s, u"12345678"_s, true);
    AUDEX_CHECK(t, !wrong.found);
    AUDEX_CHECK(t, wrong.error.isEmpty()); // a wrong entry is not a failure

    // Mixed Mode: the entry counts the data track, the title is dropped
    const Gnudb::ReadResult mixed = Gnudb::parseReadResponse(
        entry3("A / B", "Data track", "First song", "Second song"), discMixed(), u"rock"_s, u"12345678"_s, true);
    AUDEX_CHECK_MSG(t, mixed.found, mixed.error);
    if (mixed.found) {
        const Metadata::Album &a = mixed.candidate.album;
        AUDEX_EQUAL(t, a.trackNumbers().size(), qsizetype(2));
        AUDEX_CHECK(t, a.track(2).text(Field::Title) == u"First song"_s);
        AUDEX_CHECK(t, a.track(3).text(Field::Title) == u"Second song"_s);
    }

    // an entry that only counts the audio tracks fits as well
    const QByteArray audioOnly = QByteArray("210 rock 12345678 CD database entry follows (until terminating `.')\n"
                                            "# xmcd\n"
                                            "DTITLE=A / B\n"
                                            "TTITLE0=First song\nTTITLE1=Second song\n"
                                            ".\n");
    const Gnudb::ReadResult onlyAudio = Gnudb::parseReadResponse(audioOnly, discMixed(), u"rock"_s, u"12345678"_s, true);
    AUDEX_CHECK_MSG(t, onlyAudio.found, onlyAudio.error);
    if (onlyAudio.found)
        AUDEX_CHECK(t, onlyAudio.candidate.album.track(2).text(Field::Title) == u"First song"_s);
}

AUDEX_TEST("gnudb: cover art ids")
{
    const QByteArray raw = QByteArray("210 pop 12345678 CD database entry follows (until terminating `.')\n"
                                      "# xmcd\n"
                                      "# Cover: https://coverartarchive.org/release/16efdbf1-47c9-4179-ad23-4c7bc7fda547/327-500.jpg\n"
                                      "# Artid: 16efdbf1-47c9-4179-ad23-4c7bc7fda547\n"
                                      "# Artid: not-a-uuid\n"
                                      "#\n"
                                      "DTITLE=Katie Melua / Piece By Piece\n"
                                      "TTITLE0=Shy Boy\nTTITLE1=Nine Million Bicycles\nTTITLE2=Piece By Piece\n"
                                      ".\n");
    const Gnudb::ReadResult result = Gnudb::parseReadResponse(raw, disc3(), u"pop"_s, u"12345678"_s, true);
    AUDEX_CHECK_MSG(t, result.found, result.error);
    if (!result.found)
        return;
    // the id becomes the same "/front" address the MusicBrainz candidates use,
    // so the configured cover size applies; junk ids are ignored
    AUDEX_EQUAL(t, result.candidate.covers.size(), qsizetype(1));
    if (!result.candidate.covers.isEmpty()) {
        AUDEX_CHECK(t, result.candidate.covers.at(0).url == QUrl(u"https://coverartarchive.org/release/16efdbf1-47c9-4179-ad23-4c7bc7fda547/front"_s));
        AUDEX_CHECK(t, result.candidate.covers.at(0).page == QUrl(u"https://musicbrainz.org/release/16efdbf1-47c9-4179-ad23-4c7bc7fda547/cover-art"_s));
    }
}
