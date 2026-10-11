/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "encoding/tagwriter.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QtEndian>

#include <taglib/fileref.h>
#include <taglib/taglib.h>
#include <taglib/tpropertymap.h>

using namespace Audex;
using namespace Audex::Encoding;
using namespace Qt::StringLiterals;

namespace
{

// 0.1 s of silence, 44.1 kHz stereo 16 bit
bool writeWave(const QString &path)
{
    const quint32 dataSize = 4410 * 4;
    QByteArray wav("RIFF");
    auto le32 = [&wav](quint32 v) {
        char b[4];
        qToLittleEndian(v, b);
        wav.append(b, 4);
    };
    auto le16 = [&wav](quint16 v) {
        char b[2];
        qToLittleEndian(v, b);
        wav.append(b, 2);
    };
    le32(36 + dataSize);
    wav.append("WAVEfmt ");
    le32(16);
    le16(1);
    le16(2);
    le32(44100);
    le32(44100 * 4);
    le16(4);
    le16(16);
    wav.append("data");
    le32(dataSize);
    wav.append(QByteArray(dataSize, '\0'));
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(wav) == wav.size();
}

TagInfo tagInfo()
{
    Metadata::Album album;
    album.setText(Metadata::Field::Artist, u"Meat Loaf"_s);
    album.setText(Metadata::Field::Album, u"Bat Out Of Hell III"_s);
    Metadata::Track track;
    track.setText(Metadata::Field::Title, u"Blind As A Bat"_s);
    album.setTrack(2, track);

    QImage image(64, 64, QImage::Format_RGB32);
    image.fill(Qt::red);
    QByteArray jpeg;
    QBuffer buffer(&jpeg);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG");
    album.setCover({jpeg, u"image/jpeg"_s, QString(), QString(), QUrl()});

    TagInfo info;
    info.album = album;
    info.trackNumber = 2;
    info.displayTrackNumber = 2;
    info.trackTotal = 12;
    return info;
}

}

AUDEX_TEST("tags: other formats through TagLib's generic interface")
{
    QTemporaryDir dir;
    const QString path = dir.filePath(u"track.wav"_s);
    AUDEX_CHECK(t, writeWave(path));

    QString error;
    AUDEX_CHECK_MSG(t, TagWriter::write(path, u"wav"_s, tagInfo(), &error), error);

    TagLib::FileRef file(QFile::encodeName(path).constData());
    AUDEX_CHECK(t, !file.isNull());
    const TagLib::PropertyMap properties = file.file()->properties();
    AUDEX_EQUAL(t, QString::fromStdString(properties["TITLE"].toString().to8Bit(true)), u"Blind As A Bat"_s);
    AUDEX_EQUAL(t, QString::fromStdString(properties["ARTIST"].toString().to8Bit(true)), u"Meat Loaf"_s);
    AUDEX_EQUAL(t, QString::fromStdString(properties["TRACKNUMBER"].toString().to8Bit(true)), u"2/12"_s);
#if TAGLIB_MAJOR_VERSION >= 2
    AUDEX_CHECK(t, TagWriter::coverInAllFormats());
    AUDEX_EQUAL(t, qint64(file.file()->complexProperties("PICTURE").size()), qint64(1));
#else
    AUDEX_CHECK(t, !TagWriter::coverInAllFormats());
#endif
}

AUDEX_TEST("tags: unknown formats are reported")
{
    QTemporaryDir dir;
    const QString path = dir.filePath(u"track.xyz"_s);
    QFile file(path);
    AUDEX_CHECK(t, file.open(QIODevice::WriteOnly) && file.write(QByteArray(64, 'x')) == 64);
    file.close();

    QString error;
    AUDEX_CHECK(t, !TagWriter::write(path, u"xyz"_s, tagInfo(), &error));
    AUDEX_CHECK(t, !error.isEmpty());
}
