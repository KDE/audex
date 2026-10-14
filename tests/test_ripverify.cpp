/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "core/ripreport.h"
#include "online/ctdb.h"
#include "utils/imagerepair.h"
#include "utils/ripverify.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtEndian>

using namespace Audex;
using namespace Qt::StringLiterals;

namespace
{

// three tracks of 300 sectors
Cdda::Toc toc()
{
    Cdda::Toc t;
    t.tracks = {Cdda::Track{1, 1, 0, 299}, Cdda::Track{2, 1, 300, 599}, Cdda::Track{3, 1, 600, 899}};
    t.leadOutLba = 900;
    return t;
}

QByteArray disc()
{
    QByteArray data(qsizetype(900) * Cdda::SectorBytes, '\0');
    quint32 s = 4711;
    for (qsizetype i = 0; i < data.size() / 2; ++i) {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        qToLittleEndian<quint16>(quint16(s >> 8), data.data() + 2 * i);
    }
    return data;
}

bool writeWav(const QString &path, const QByteArray &audio)
{
    Encoding::WavEncoderFactory factory;
    const auto encoder = factory.create({});
    QString error;
    return encoder->open(path, audio.size() / 4, &error) && encoder->write(audio, &error) && encoder->finish(&error);
}

struct Layout {
    int track;
    int firstLba; // of the file
    int lastLba;
};

// Rips the disc into WAVE files (one per entry; equal names: one image) and
// writes the log as the rip would; returns the log path
QString rip(const QDir &dir, const QList<Layout> &layout, bool image = false)
{
    static const Encoding::WavEncoderFactory factory;
    const QByteArray audio = disc();
    const Cdda::Toc t = toc();
    QList<DiscAudioFile> files;
    QList<Rip::Segment> segments;
    QStringList names;
    for (const Layout &l : layout) {
        const QString path = dir.filePath(image ? u"disc.wav"_s : u"%1.wav"_s.arg(l.track, 2, 10, QLatin1Char('0')));
        if (!image || files.isEmpty()) {
            const int first = image ? layout.first().firstLba : l.firstLba;
            const int last = image ? layout.last().lastLba : l.lastLba;
            writeWav(path, audio.mid(qsizetype(first) * Cdda::SectorBytes, qsizetype(last - first + 1) * Cdda::SectorBytes));
            files << DiscAudioFile{path, &factory, first, last};
        }
        Rip::Segment s;
        s.trackNumber = l.track;
        s.firstLba = l.firstLba;
        s.lastLba = l.lastLba;
        const Cdda::Track *disc = t.track(l.track);
        if (disc->firstLba != l.firstLba)
            s.checksumFirstLba = disc->firstLba;
        if (disc->lastLba != l.lastLba)
            s.checksumLastLba = disc->lastLba;
        s.accurateRipFirst = l.track == 1;
        s.accurateRipLast = l.track == 3;
        s.ctdbSkipFirst = s.accurateRipFirst ? Ctdb::StrideSamples : 0;
        s.ctdbSkipLast = s.accurateRipLast ? Ctdb::skipLast(t) : 0;
        segments << s;
        names << path;
    }
    QString error;
    Rip::RipResult result;
    result.segments = *checksumFiles(files, segments, Rip::RipOptions(), &error);
    result.completed = true;
    Rip::ReportContext ctx;
    ctx.application = u"Audex 26.11.70"_s;
    ctx.options.mode = Rip::ReadMode::Secure;
    ctx.toc = t;
    ctx.fileNames = names;
    const QString logPath = dir.filePath(u"rip.log"_s);
    QFile log(logPath);
    if (log.open(QIODevice::WriteOnly)) {
        log.write(Rip::formatReport(ctx, result).join(u'\n').toUtf8());
    }
    return logPath;
}

Verify::Result check(const QString &logPath, const QString &directory = {})
{
    Verify::Request rq;
    rq.logPath = logPath;
    rq.directory = directory;
    rq.encoders = std::make_shared<Encoding::EncoderRegistry>();
    rq.online = false;
    return Verify::verifyRip(rq);
}

void flipByte(const QString &path, qint64 at)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadWrite)) {
        return;
    }
    f.seek(at);
    char c = 0;
    f.getChar(&c);
    f.seek(at);
    f.putChar(char(c ^ 0x5a));
}

const QList<Layout> Tracks{{1, 0, 299}, {2, 300, 599}, {3, 600, 899}};

}

AUDEX_TEST("verify: the log of a rip is read back")
{
    QTemporaryDir dir;
    QFile file(rip(QDir(dir.path()), Tracks));
    AUDEX_CHECK(t, file.open(QIODevice::ReadOnly));
    QString error;
    const auto log = Verify::parseRipLog(QString::fromUtf8(file.readAll()), &error);
    AUDEX_CHECK_MSG(t, log.has_value(), error);
    AUDEX_EQUAL(t, log->application, u"Audex 26.11.70"_s);
    AUDEX_CHECK(t, log->toc.tracks == toc().tracks);
    AUDEX_EQUAL(t, qint64(log->toc.leadOutLba), qint64(900));
    AUDEX_EQUAL(t, qint64(log->tracks.size()), qint64(3));
    AUDEX_EQUAL(t, log->tracks.at(1).file, dir.filePath(u"02.wav"_s));
    AUDEX_EQUAL(t, qint64(log->tracks.at(1).firstLba), qint64(300));
    AUDEX_CHECK(t, log->tracks.at(2).copyCrc.has_value());

    AUDEX_CHECK(t, !Verify::parseRipLog(u"some text"_s, &error).has_value());
}

AUDEX_TEST("verify: unchanged track files, then a changed and a missing one")
{
    QTemporaryDir dir;
    const QString logPath = rip(QDir(dir.path()), Tracks);

    Verify::Result result = check(logPath);
    AUDEX_EQUAL(t, Verify::verdictName(result.verdict), u"ok"_s);
    for (const Verify::TrackResult &track : std::as_const(result.tracks))
        AUDEX_CHECK(t, track.crc && !track.changed());

    flipByte(dir.filePath(u"02.wav"_s), 44 + 1000);
    result = check(logPath);
    AUDEX_EQUAL(t, Verify::verdictName(result.verdict), u"failed"_s);
    AUDEX_CHECK(t, !result.tracks.at(0).changed() && result.tracks.at(1).changed());
    AUDEX_CHECK(t, result.report.join(u'\n').contains(u"CHANGED"_s));

    QFile::remove(dir.filePath(u"03.wav"_s));
    flipByte(dir.filePath(u"02.wav"_s), 44 + 1000); // back
    result = check(logPath);
    AUDEX_EQUAL(t, Verify::verdictName(result.verdict), u"error"_s);
    AUDEX_CHECK(t, !result.tracks.at(2).problem.isEmpty());
}

AUDEX_TEST("verify: a moved rip, an image and pregaps with their own track")
{
    QTemporaryDir dir;
    QDir(dir.path()).mkdir(u"old"_s);
    rip(QDir(dir.filePath(u"old"_s)), Tracks);
    QDir(dir.path()).rename(u"old"_s, u"new"_s); // the paths in the log are gone
    AUDEX_EQUAL(t, Verify::verdictName(check(dir.filePath(u"new/rip.log"_s)).verdict), u"ok"_s);

    QTemporaryDir image;
    AUDEX_EQUAL(t, Verify::verdictName(check(rip(QDir(image.path()), Tracks, true)).verdict), u"ok"_s);

    // track 2's pregap (280-299) and track 3's (590-599) start their own files
    QTemporaryDir own;
    const QList<Layout> layout{{1, 0, 279}, {2, 280, 589}, {3, 590, 899}};
    const Verify::Result result = check(rip(QDir(own.path()), layout));
    AUDEX_EQUAL(t, Verify::verdictName(result.verdict), u"ok"_s);
}
