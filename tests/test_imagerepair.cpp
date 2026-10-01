/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Repair of an image file with CTDB recovery data. The database entry and its
// recovery data are made from the undamaged audio; nothing is downloaded.

#include "test_framework.h"

#include "encoding/registry.h"
#include "sim/simulatedsectorreader.h"
#include "utils/imagerepair.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtEndian>

using namespace Audex;
using namespace Qt::StringLiterals;

namespace
{

constexpr int WordsPerSector = Cdda::SectorBytes / 2;

// 32 sectors of hidden track one audio, then three tracks
Cdda::Toc toc()
{
    Cdda::Toc t;
    t.tracks = {Cdda::Track{1, 1, 32, 699}, Cdda::Track{2, 1, 700, 1499}, Cdda::Track{3, 1, 1500, 2344}};
    t.leadOutLba = 2345;
    return t;
}

QByteArray pcm(int sectors, quint32 seed)
{
    QByteArray data(qsizetype(sectors) * Cdda::SectorBytes, '\0');
    quint32 s = seed;
    for (qsizetype i = 0; i < data.size() / 2; ++i) {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        qToLittleEndian<quint16>(quint16(s >> 8), data.data() + 2 * i);
    }
    return data;
}

void damage(QByteArray &data, qint64 word, qint64 count)
{
    for (qint64 i = word; i < word + count; ++i)
        qToLittleEndian<quint16>(qFromLittleEndian<quint16>(data.constData() + 2 * i) ^ 0x5a5a, data.data() + 2 * i);
}

bool writeWav(const QString &path, const QByteArray &audio)
{
    Encoding::WavEncoderFactory factory;
    const auto encoder = factory.create({});
    QString error;
    return encoder->open(path, audio.size() / 4, &error) && encoder->write(audio, &error) && encoder->finish(&error);
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

// database entry of the pressing whose sample i is sample i + pressing of `audio`
struct Database {
    Ctdb::Entry entry;
    QByteArray parity;
};

Database database(const QByteArray &audio, int pressing)
{
    const Cdda::Toc t = toc();
    QByteArray moved(audio.size(), '\0');
    const qint64 words = audio.size() / 2;
    for (qint64 i = 0; i < words; ++i) {
        const qint64 j = i + 2 * qint64(pressing);
        if (j >= 0 && j < words)
            std::memcpy(moved.data() + 2 * i, audio.constData() + 2 * j, 2);
    }
    Ctdb::Parity parity(32 * WordsPerSector, qint64(2345 - 32) * WordsPerSector);
    parity.update(moved);

    Database db;
    db.entry.crc = parity.crc(0);
    db.entry.confidence = 5;
    db.entry.stride = Ctdb::StrideSamples;
    db.entry.npar = Ctdb::MaxParity;
    db.entry.toc = Ctdb::tocString(t);
    db.entry.parityUrl = u"http://example.invalid/parity"_s;
    const Ctdb::ColumnSyndromes s = parity.syndromes(0, 0);
    db.entry.syndromes = QList<quint16>(s.cbegin(), s.cend());
    db.parity = parity.syndromeData(Ctdb::MaxParity);
    return db;
}

ImageRepairRequest request(const QString &path, const Database &db, const Encoding::EncoderFactory &factory)
{
    ImageRepairRequest rq;
    rq.path = path;
    rq.encoder = &factory;
    rq.writeTags = false;
    rq.toc = toc();
    rq.firstLba = 0;
    rq.entries = {db.entry};
    rq.fetchParity = [parity = db.parity](const Ctdb::Entry &, QString *) {
        return parity;
    };
    return rq;
}

}

AUDEX_TEST("image repair restores a damaged WAVE image")
{
    QTemporaryDir dir;
    const QString path = dir.filePath(u"disc.wav"_s);
    const QString cleanPath = dir.filePath(u"clean.wav"_s);
    const QByteArray clean = pcm(2345, 99);
    QByteArray rip = clean;
    damage(rip, 1000 * WordsPerSector + 11, 30 * WordsPerSector); // 30 sectors
    damage(rip, 2000 * WordsPerSector + 5, 1);
    AUDEX_CHECK(t, writeWav(path, rip) && writeWav(cleanPath, clean));
    const QByteArray damaged = readFile(path);

    Encoding::WavEncoderFactory factory;
    for (int pressing : {0, -2345}) {
        AUDEX_CHECK(t, writeWav(path, rip));
        ImageRepairRequest rq = request(path, database(clean, pressing), factory);
        rq.keepOriginal = pressing != 0;
        const ImageRepairResult result = repairImage(rq);

        AUDEX_CHECK_MSG(t, result.repaired, result.log.join(u'\n'));
        AUDEX_EQUAL(t, result.corrections, 30 * WordsPerSector + 1);
        AUDEX_EQUAL_DATA(t, readFile(path), readFile(cleanPath));
        AUDEX_CHECK(t, !QFile::exists(QDir(dir.path()).filePath(u"disc.part.wav"_s)));
        if (rq.keepOriginal)
            AUDEX_EQUAL_DATA(t, readFile(result.originalPath), damaged);
        else
            AUDEX_CHECK(t, !QFile::exists(unrepairedPath(path)));
        AUDEX_CHECK_MSG(t, pressing == 0 || result.log.join(u'\n').contains(u"other pressing with offset -2345"_s), result.log.join(u'\n'));
    }
}

AUDEX_TEST("image repair leaves an image alone it cannot restore")
{
    QTemporaryDir dir;
    const QString path = dir.filePath(u"disc.wav"_s);
    const QByteArray clean = pcm(2345, 99);
    QByteArray rip = clean;
    damage(rip, 1000 * WordsPerSector + 11, 100 * WordsPerSector); // too much
    AUDEX_CHECK(t, writeWav(path, rip));
    const QByteArray before = readFile(path);

    Encoding::WavEncoderFactory factory;
    const ImageRepairResult result = repairImage(request(path, database(clean, 0), factory));
    AUDEX_CHECK(t, !result.repaired);
    AUDEX_EQUAL_DATA(t, readFile(path), before);
    AUDEX_CHECK_MSG(t, result.log.last().contains(u"could not be repaired"_s), result.log.join(u'\n'));

    // the database confirms an undamaged image: nothing is downloaded or written
    AUDEX_CHECK(t, writeWav(path, clean));
    ImageRepairRequest rq = request(path, database(clean, 0), factory);
    bool downloaded = false;
    rq.fetchParity = [&downloaded](const Ctdb::Entry &, QString *) {
        downloaded = true;
        return QByteArray();
    };
    const ImageRepairResult unchanged = repairImage(rq);
    AUDEX_CHECK(t, !unchanged.repaired && !downloaded);
    AUDEX_CHECK_MSG(t, unchanged.log.last().contains(u"nothing to repair"_s), unchanged.log.join(u'\n'));
}

AUDEX_TEST("tracks read again replace their first read in the image")
{
    QTemporaryDir dir;
    const QString path = dir.filePath(u"disc.wav"_s);
    const QByteArray clean = pcm(2345, 7);
    QByteArray rip = clean;
    // tracks 1 (sectors 32..699) and 3 (1500..2344) were wrong after the fast pass
    damage(rip, 100 * WordsPerSector, 50 * WordsPerSector);
    damage(rip, 2000 * WordsPerSector, 20);
    AUDEX_CHECK(t, writeWav(path, rip));

    const auto reread = [&](int first, int last, const QString &name) {
        const QString file = dir.filePath(name);
        QFile out(file);
        if (out.open(QIODevice::WriteOnly))
            out.write(clean.mid(qint64(first) * Cdda::SectorBytes, qint64(last - first + 1) * Cdda::SectorBytes));
        return ImagePatch{qint64(first) * Cdda::SectorBytes, file};
    };

    Encoding::WavEncoderFactory factory;
    ImagePatchRequest rq;
    rq.path = path;
    rq.encoder = &factory;
    rq.writeTags = false;
    rq.patches = {reread(32, 699, u"t1.pcm"_s), reread(1500, 2344, u"t3.pcm"_s)};
    QString error;
    AUDEX_CHECK_MSG(t, patchImage(rq, &error), error);

    const QString cleanPath = dir.filePath(u"clean.wav"_s);
    AUDEX_CHECK(t, writeWav(cleanPath, clean));
    AUDEX_EQUAL_DATA(t, readFile(path), readFile(cleanPath));
    AUDEX_CHECK(t, !QFile::exists(QDir(dir.path()).filePath(u"disc.part.wav"_s)));

    // audio beyond the end of the image is refused, the image stays as it is
    const QByteArray before = readFile(path);
    rq.patches = {reread(2300, 2344, u"end.pcm"_s)};
    rq.patches.first().offsetBytes += Cdda::SectorBytes;
    AUDEX_CHECK(t, !patchImage(rq, &error));
    AUDEX_EQUAL_DATA(t, readFile(path), before);
}

AUDEX_TEST("an image read back has the checksums of its extraction")
{
    // extraction of a disc into an image (with the shifted checksums of
    // another pressing and the CTDB margins), then the image read back
    const auto disc = Sim::SimulatedDisc::generate({700, 800, 845}, 32, 5);
    Sim::DriveModel model;
    model.seed = 3;
    Sim::SimulatedSectorReader reader(disc, model);

    QList<Rip::Segment> segments;
    for (const Cdda::Track &track : disc.toc.tracks) {
        Rip::Segment s;
        s.trackNumber = track.number;
        s.firstLba = track.firstLba;
        s.lastLba = track.lastLba;
        s.accurateRipFirst = track.number == 1;
        s.accurateRipLast = track.number == 3;
        s.ctdbSkipFirst = s.accurateRipFirst ? Ctdb::StrideSamples : 0;
        segments << s;
    }
    Rip::RipOptions options;
    options.mode = Rip::ReadMode::Fast;
    options.accurateRipShifts = {30, -12};
    options.ctdbShiftRange = 64;

    QByteArray audio;
    Rip::RipCallbacks callbacks;
    callbacks.write = [&](int, QByteArrayView pcm) {
        audio.append(pcm.data(), pcm.size());
        return true;
    };
    const Rip::ReadableArea area{segments.first().firstLba, segments.last().lastLba + 1};
    Rip::RipEngine engine(reader, area, options, std::move(callbacks));
    const Rip::RipResult ripped = engine.run(segments);
    AUDEX_CHECK_MSG(t, ripped.completed, ripped.error);

    QTemporaryDir dir;
    const QString path = dir.filePath(u"disc.wav"_s);
    AUDEX_CHECK(t, writeWav(path, audio));

    Encoding::WavEncoderFactory factory;
    ImageChecksumRequest rq;
    rq.path = path;
    rq.encoder = &factory;
    rq.segments = segments;
    rq.options = options;
    QString error;
    const auto checked = checksumImage(rq, &error);
    AUDEX_CHECK_MSG(t, checked.has_value(), error);
    AUDEX_EQUAL(t, checked->size(), ripped.segments.size());
    for (qsizetype i = 0; i < ripped.segments.size(); ++i) {
        const Rip::SegmentResult &a = ripped.segments.at(i);
        const Rip::SegmentResult &b = checked->at(i);
        AUDEX_EQUAL(t, b.crc32, a.crc32);
        AUDEX_EQUAL(t, b.crc32NoNull, a.crc32NoNull);
        AUDEX_EQUAL(t, b.accurateRipV1, a.accurateRipV1);
        AUDEX_EQUAL(t, b.accurateRipV2, a.accurateRipV2);
        AUDEX_EQUAL(t, b.accurateRipFrame450, a.accurateRipFrame450);
        AUDEX_EQUAL(t, b.ctdbCrc, a.ctdbCrc);
        AUDEX_EQUAL_DATA(t, b.ctdbHead, a.ctdbHead);
        AUDEX_EQUAL_DATA(t, b.ctdbTail, a.ctdbTail);
        AUDEX_EQUAL(t, b.accurateRipShifted.size(), a.accurateRipShifted.size());
        for (qsizetype j = 0; j < a.accurateRipShifted.size() && j < b.accurateRipShifted.size(); ++j) {
            AUDEX_EQUAL(t, b.accurateRipShifted.at(j).v1, a.accurateRipShifted.at(j).v1);
            AUDEX_EQUAL(t, b.accurateRipShifted.at(j).v2, a.accurateRipShifted.at(j).v2);
            AUDEX_EQUAL(t, b.accurateRipShifted.at(j).ctdb, a.accurateRipShifted.at(j).ctdb);
        }
    }

    // a changed sample changes its track only
    QByteArray changed = audio;
    const qint64 at = qint64(700 + 32 + 100) * Cdda::SectorBytes; // track 2
    changed[at] = char(changed.at(at) ^ 0x11);
    AUDEX_CHECK(t, writeWav(path, changed));
    const auto again = checksumImage(rq, &error);
    AUDEX_CHECK_MSG(t, again.has_value(), error);
    AUDEX_EQUAL(t, again->at(0).crc32, ripped.segments.at(0).crc32);
    AUDEX_CHECK(t, again->at(1).crc32 != ripped.segments.at(1).crc32);
    AUDEX_EQUAL(t, again->at(2).crc32, ripped.segments.at(2).crc32);

    // an image shorter than the disc is refused
    AUDEX_CHECK(t, writeWav(path, audio.first(audio.size() - 10 * Cdda::SectorBytes)));
    AUDEX_CHECK(t, !checksumImage(rq, &error).has_value());
}
