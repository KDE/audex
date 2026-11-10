/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "encoding/tagwriter.h"
#include "models/cdinfomodel.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QMimeDatabase>
#include <QTemporaryDir>

using namespace Audex;
using namespace Audex::Encoding;
using namespace Qt::StringLiterals;

namespace
{

Metadata::CoverArt cover(int size, const char *format, bool transparent = false)
{
    QImage image(size, size / 2, transparent ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    image.fill(transparent ? QColor(255, 0, 0, 128) : QColor(200, 100, 50));
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, format);
    return {data, u"image/"_s + QString::fromLatin1(format).toLower(), QString(), QString(), QUrl()};
}

}

AUDEX_TEST("cover: kept when format and size fit")
{
    const Metadata::CoverArt jpeg = cover(800, "JPEG");
    const auto prepared = TagWriter::prepareCover(jpeg, 1000, CoverFormat::Original);
    AUDEX_CHECK(t, prepared && prepared->cover.data == jpeg.data);
    const Metadata::CoverArt png = cover(800, "PNG");
    AUDEX_CHECK(t, TagWriter::prepareCover(png, 1000, CoverFormat::Original)->cover.data == png.data);
    AUDEX_CHECK(t, TagWriter::prepareCover(png, 0, CoverFormat::Png)->cover.data == png.data);
}

AUDEX_TEST("cover: scaled and converted as asked")
{
    const auto scaled = TagWriter::prepareCover(cover(2000, "PNG"), 1000, CoverFormat::Original);
    AUDEX_EQUAL(t, qint64(scaled->width), qint64(1000));
    AUDEX_EQUAL(t, scaled->cover.mimeType, u"image/png"_s);

    const auto jpeg = TagWriter::prepareCover(cover(800, "PNG"), 1000, CoverFormat::Jpeg);
    AUDEX_EQUAL(t, jpeg->cover.mimeType, u"image/jpeg"_s);
    AUDEX_EQUAL(t, qint64(jpeg->width), qint64(800));

    const auto png = TagWriter::prepareCover(cover(800, "JPEG"), 0, CoverFormat::Png);
    AUDEX_EQUAL(t, png->cover.mimeType, u"image/png"_s);
}

AUDEX_TEST("cover: other formats and transparency")
{
    AUDEX_EQUAL(t, TagWriter::prepareCover(cover(300, "BMP"), 0, CoverFormat::Original)->cover.mimeType, u"image/jpeg"_s);

    const auto flattened = TagWriter::prepareCover(cover(300, "PNG", true), 0, CoverFormat::Jpeg);
    AUDEX_EQUAL(t, flattened->cover.mimeType, u"image/jpeg"_s);
    const QImage image = QImage::fromData(flattened->cover.data);
    AUDEX_CHECK_MSG(t, qGreen(image.pixel(10, 10)) > 100, u"transparency must become white, not black"_s);
}

AUDEX_TEST("cover: lossless savings when re-encoded")
{
    QImage opaque(400, 400, QImage::Format_ARGB32);
    opaque.fill(QColor(200, 100, 50));
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    opaque.save(&buffer, "PNG");
    const auto png = TagWriter::prepareCover({data, u"image/png"_s, QString(), QString(), QUrl()}, 200, CoverFormat::Png);
    AUDEX_CHECK(t, !QImage::fromData(png->cover.data).hasAlphaChannel());

    QImage gray(400, 400, QImage::Format_RGB32);
    gray.fill(QColor(90, 90, 90));
    data.clear();
    buffer.seek(0);
    gray.save(&buffer, "PNG");
    const auto grayPng = TagWriter::prepareCover({data, u"image/png"_s, QString(), QString(), QUrl()}, 200, CoverFormat::Png);
    AUDEX_CHECK(t, QImage::fromData(grayPng->cover.data).isGrayscale());
}

AUDEX_TEST("cover: saved in the format the file name asks for")
{
    QTemporaryDir dir;
    CDInfoModel model;
    const Metadata::CoverArt jpeg = cover(64, "JPEG");
    model.setCover(jpeg);
    const auto read = [](const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    const auto type = [&](const QString &path) {
        return QMimeDatabase().mimeTypeForData(read(path)).name();
    };

    // same format or no image suffix: the bytes as they are
    AUDEX_CHECK(t, model.saveCoverToFile(dir.filePath(u"a.jpg"_s)));
    AUDEX_CHECK(t, read(dir.filePath(u"a.jpg"_s)) == jpeg.data);
    AUDEX_CHECK(t, model.saveCoverToFile(dir.filePath(u"a.cover"_s)));
    AUDEX_CHECK(t, read(dir.filePath(u"a.cover"_s)) == jpeg.data);

    // another format: converted
    AUDEX_CHECK(t, model.saveCoverToFile(dir.filePath(u"b.png"_s)));
    AUDEX_EQUAL(t, type(dir.filePath(u"b.png"_s)), u"image/png"_s);

    // transparency on white for JPEG
    model.setCover(cover(64, "PNG", true));
    AUDEX_CHECK(t, model.saveCoverToFile(dir.filePath(u"c.jpg"_s)));
    AUDEX_EQUAL(t, type(dir.filePath(u"c.jpg"_s)), u"image/jpeg"_s);
    const QImage opaque(dir.filePath(u"c.jpg"_s));
    AUDEX_CHECK(t, !opaque.hasAlphaChannel() && qRed(opaque.pixel(0, 0)) > 240 && qGreen(opaque.pixel(0, 0)) > 100);

    AUDEX_CHECK(t, !model.saveCoverToFile(dir.filePath(u"missing/d.png"_s)));
}
