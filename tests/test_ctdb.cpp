/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "core/checksums.h"
#include "core/ripengine.h"
#include "online/blockingfetch.h"
#include "online/ctdb.h"
#include "sim/simulatedsectorreader.h"

#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

using namespace Audex;
using namespace Audex::Rip;
using namespace Audex::Sim;
using namespace Qt::StringLiterals;

AUDEX_TEST("CTDB uses HTTPS and restricts recovery URLs")
{
    const SimulatedDisc disc = SimulatedDisc::generate({500}, 0, 1);
    const QUrl lookup = Ctdb::lookupUrl(disc.toc);
    AUDEX_EQUAL(t, lookup.scheme(), u"https"_s);
    AUDEX_EQUAL(t, lookup.host(), u"db.cue.tools"_s);
    AUDEX_EQUAL(t, lookup.path(), u"/lookup2.php"_s);

    const auto parity = [](const QString &address) {
        const QByteArray xml = u"<ctdb><entry crc32=\"12345678\" toc=\"0:500\" npar=\"1\" syndrome=\"AQA=\" hasparity=\"%1\"/></ctdb>"_s
                                   .arg(address.toHtmlEscaped()).toUtf8();
        const QList<Ctdb::Entry> entries = Ctdb::parseResponse(xml);
        return entries.isEmpty() ? Ctdb::Entry{} : entries.first();
    };
    AUDEX_EQUAL(t, parity(u"/parity/123"_s).parityUrl, u"https://db.cue.tools/parity/123"_s);
    AUDEX_EQUAL(t, parity(u"parity/123"_s).parityUrl, u"https://db.cue.tools/parity/123"_s);
    AUDEX_EQUAL(t, parity(u"https://db.cue.tools/parity/123"_s).parityUrl, u"https://db.cue.tools/parity/123"_s);
    AUDEX_EQUAL(t, parity(u"http://p.cuetools.net/123"_s).parityUrl, u"http://p.cuetools.net/123"_s);

    const QStringList rejected = {
        u"http://db.cuetools.net/parity/123"_s,
        u"http://db.cue.tools/parity/123"_s,
        u"https://evil.example/parity/123"_s,
        u"http://p.cuetools.net.evil.example/123"_s,
        u"http://p.cuetools.net:8080/123"_s,
        u"http://user@p.cuetools.net/123"_s,
        u"file:///etc/passwd"_s,
        u"//evil.example/parity/123"_s,
    };
    for (const QString &address : rejected) {
        const Ctdb::Entry entry = parity(address);
        AUDEX_CHECK_MSG(t, !entry.hasParity(), address);
        QString error;
        Ctdb::Entry forged = entry;
        forged.parityUrl = address;
        AUDEX_CHECK(t, Ctdb::fetchParity(forged, &error).isEmpty());
        AUDEX_CHECK_MSG(t, !error.isEmpty(), address);
    }
}

AUDEX_TEST("CTDB redirect policy checks every destination")
{
    QTcpServer server;
    AUDEX_CHECK(t, server.listen(QHostAddress::LocalHost));
    if (!server.isListening())
        return;

    int requests = 0;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&] {
        while (QTcpSocket *socket = server.nextPendingConnection()) {
            ++requests;
            if (requests == 1 || requests == 2)
                socket->write("HTTP/1.1 302 Found\r\nLocation: /result\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            else
                socket->write("HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK");
            socket->disconnectFromHost();
        }
    });

    const QUrl url(u"http://127.0.0.1:%1/test"_s.arg(server.serverPort()));
    QString error;
    const QByteArray blocked = Online::fetchBlocking(url, &error, nullptr, 5000, {}, {}, [](const QUrl &) {
        return false;
    });
    AUDEX_CHECK(t, blocked.isEmpty());
    AUDEX_CHECK_MSG(t, error.contains(u"redirect"_s), error);
    AUDEX_EQUAL(t, requests, 1);

    const QByteArray followed = Online::fetchBlocking(url, &error, nullptr, 5000, {}, {}, [&](const QUrl &next) {
        return next.scheme() == u"http"_s && next.host() == u"127.0.0.1"_s && next.port() == server.serverPort();
    });
    AUDEX_EQUAL_DATA(t, followed, QByteArray("OK"));
    AUDEX_CHECK_MSG(t, error.isEmpty(), error);
    AUDEX_EQUAL(t, requests, 3);
}

AUDEX_TEST("CTDB finds another pressing without AccurateRip")
{
    const SimulatedDisc disc = SimulatedDisc::generate({600, 500, 400}, 0, 11);
    const Cdda::Toc &toc = disc.toc;
    SimulatedSectorReader reader(disc, DriveModel{});

    QList<Segment> segments;
    for (const Cdda::Track &track : toc.tracks) {
        Segment s;
        s.trackNumber = track.number;
        s.firstLba = track.firstLba;
        s.lastLba = track.lastLba;
        s.accurateRipFirst = track.number == toc.firstAudioTrackNumber();
        s.accurateRipLast = track.number == toc.lastAudioTrackNumber();
        s.ctdbSkipFirst = s.accurateRipFirst ? Ctdb::StrideSamples : 0;
        s.ctdbSkipLast = s.accurateRipLast ? Ctdb::skipLast(toc) : 0;
        segments.append(s);
    }
    RipOptions options;
    options.mode = ReadMode::Fast;
    options.cacheDefeat = false;
    options.ctdbShiftRange = Ctdb::ShiftRange;
    RipEngine engine(reader, {toc.audioStartLba(), toc.audioEndLba()}, options, {});
    const RipResult result = engine.run(segments);
    AUDEX_CHECK_MSG(t, result.completed, result.error);

    // the database knows a pressing whose sample i is our sample i + shift
    constexpr int shift = -1234;
    const auto crcOf = [&](qint64 fromSample, qint64 toSample) {
        return Crc32::compute(QByteArrayView(disc.audio).sliced((fromSample + shift) * 4, (toSample - fromSample) * 4));
    };
    Ctdb::Entry entry;
    entry.confidence = 7;
    entry.stride = Ctdb::StrideSamples;
    entry.toc = Ctdb::tocString(toc);
    const qint64 end = qint64(toc.audioEndLba()) * Cdda::SamplesPerSector;
    entry.crc = crcOf(Ctdb::StrideSamples, end - Ctdb::skipLast(toc));
    for (const Segment &s : std::as_const(segments))
        entry.trackCrcs.append(
            crcOf(qint64(s.firstLba) * Cdda::SamplesPerSector + s.ctdbSkipFirst, qint64(s.lastLba + 1) * Cdda::SamplesPerSector - s.ctdbSkipLast));

    int mismatches = -1;
    const QStringList lines = Ctdb::formatVerification({entry}, result.segments, toc, &mismatches);
    AUDEX_EQUAL(t, mismatches, 0);
    AUDEX_CHECK_MSG(t, lines.join(u'\n').contains(QStringLiteral("other pressing with offset -1234")), lines.join(u'\n'));
    AUDEX_CHECK_MSG(t, lines.last() == QStringLiteral("All tracks accurately ripped."), lines.last());

    // one track differs: the others still reveal the offset
    entry.crc ^= 1;
    entry.trackCrcs[1] ^= 1;
    const QStringList partial = Ctdb::formatVerification({entry}, result.segments, toc, &mismatches);
    AUDEX_EQUAL(t, mismatches, 1);
    AUDEX_CHECK_MSG(t, partial.join(u'\n').contains(QStringLiteral("Track 01: accurately ripped")), partial.join(u'\n'));
    AUDEX_CHECK_MSG(t, partial.join(u'\n').contains(QStringLiteral("Track 02: NOT accurately ripped")), partial.join(u'\n'));
}

AUDEX_TEST("CTDB confirms single tracks at the read offset and at AccurateRip shifts")
{
    const SimulatedDisc disc = SimulatedDisc::generate({600, 500, 400}, 0, 12);
    const Cdda::Toc &toc = disc.toc;

    QList<Segment> segments;
    for (const Cdda::Track &track : toc.tracks) {
        Segment s;
        s.trackNumber = track.number;
        s.firstLba = track.firstLba;
        s.lastLba = track.lastLba;
        s.accurateRipFirst = track.number == toc.firstAudioTrackNumber();
        s.accurateRipLast = track.number == toc.lastAudioTrackNumber();
        s.ctdbSkipFirst = s.accurateRipFirst ? Ctdb::StrideSamples : 0;
        s.ctdbSkipLast = s.accurateRipLast ? Ctdb::skipLast(toc) : 0;
        segments.append(s);
    }
    const auto rip = [&](const QList<int> &accurateRipShifts) {
        SimulatedSectorReader reader(disc, DriveModel{});
        RipOptions options;
        options.mode = ReadMode::Fast;
        options.cacheDefeat = false;
        options.accurateRipShifts = accurateRipShifts;
        RipEngine engine(reader, {toc.audioStartLba(), toc.audioEndLba()}, options, {});
        return engine.run(segments);
    };

    // entry of the pressing whose sample i is our sample i + shift
    const auto entryAt = [&](int shift, int confidence) {
        Ctdb::Entry e;
        e.confidence = confidence;
        e.stride = Ctdb::StrideSamples;
        e.toc = Ctdb::tocString(toc);
        for (const Segment &s : std::as_const(segments)) {
            const qint64 from = qint64(s.firstLba) * Cdda::SamplesPerSector + s.ctdbSkipFirst + shift;
            const qint64 to = qint64(s.lastLba + 1) * Cdda::SamplesPerSector - s.ctdbSkipLast + shift;
            e.trackCrcs.append(Crc32::compute(QByteArrayView(disc.audio).sliced(from * 4, (to - from) * 4)));
        }
        return e;
    };
    constexpr int shift = -1234;
    const Ctdb::Entry same = entryAt(0, 3);
    const Ctdb::Entry other = entryAt(shift, 4);

    const RipResult plain = rip({});
    AUDEX_CHECK_MSG(t, plain.completed, plain.error);
    for (const SegmentResult &r : plain.segments) {
        AUDEX_EQUAL(t, Ctdb::trackConfidence({same}, r, toc), 3);
        AUDEX_EQUAL(t, Ctdb::trackConfidence({same, other}, r, toc), 3); // no search over all shifts
    }

    // the shift of another pressing found via AccurateRip is taken into account
    const RipResult shifted = rip({shift});
    AUDEX_CHECK_MSG(t, shifted.completed, shifted.error);
    for (const SegmentResult &r : shifted.segments)
        AUDEX_EQUAL(t, Ctdb::trackConfidence({same, other}, r, toc), 7);

    // a damaged track is not confirmed, the others still are
    Ctdb::Entry damaged = same;
    damaged.trackCrcs[1] ^= 1;
    AUDEX_EQUAL(t, Ctdb::trackConfidence({damaged}, plain.segments.at(0), toc), 3);
    AUDEX_EQUAL(t, Ctdb::trackConfidence({damaged}, plain.segments.at(1), toc), 0);

    // entries that cannot confirm single tracks
    Ctdb::Entry old = same;
    old.trackCrcs.clear();
    Ctdb::Entry otherAudio = same;
    otherAudio.stride = 2 * Ctdb::StrideSamples;
    AUDEX_CHECK(t, Ctdb::canConfirmTracks({same}, toc));
    AUDEX_CHECK(t, !Ctdb::canConfirmTracks({old, otherAudio}, toc));
    AUDEX_EQUAL(t, Ctdb::trackConfidence({old, otherAudio}, plain.segments.at(0), toc), 0);
}
