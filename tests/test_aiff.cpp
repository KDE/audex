/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// The AIFF plugin, loaded from the build tree like the application does

#include "test_framework.h"

#include "encoding/registry.h"
#include "encoding/tagwriter.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtEndian>

#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>

using namespace Audex;
using namespace Audex::Encoding;
using namespace Qt::StringLiterals;

namespace
{

EncoderFactory *aiffFactory(EncoderRegistry &registry)
{
    registry.loadPlugins({QStringLiteral(AUDEX_TEST_PLUGIN_DIR)});
    return registry.factory(u"aiff"_s);
}

// frames of a rising pattern, little endian as the engine delivers it
QByteArray pcm(int frames)
{
    QByteArray data(frames * BytesPerFrame, '\0');
    for (int i = 0; i < frames * Channels; ++i)
        qToLittleEndian<qint16>(qint16(i * 257 - 30000), data.data() + 2 * i);
    return data;
}

}

AUDEX_TEST("aiff: header, big endian samples and reading back")
{
    EncoderRegistry registry;
    EncoderFactory *factory = aiffFactory(registry);
    AUDEX_CHECK(t, factory != nullptr);
    if (!factory)
        return;

    QTemporaryDir dir;
    const QString path = dir.filePath(u"track.aiff"_s);
    const QByteArray audio = pcm(1000);
    QString error;
    std::unique_ptr<AudioEncoder> encoder = factory->create({});
    AUDEX_CHECK_MSG(t, encoder->open(path, 1000, &error), error);
    AUDEX_CHECK_MSG(t, encoder->write(QByteArrayView(audio).first(1600), &error), error);
    AUDEX_CHECK_MSG(t, encoder->write(QByteArrayView(audio).sliced(1600), &error), error);
    AUDEX_CHECK_MSG(t, encoder->finish(&error), error);

    QFile file(path);
    AUDEX_CHECK(t, file.open(QIODevice::ReadOnly));
    const QByteArray bytes = file.readAll();
    AUDEX_EQUAL(t, bytes.size(), qsizetype(54 + 4000));
    AUDEX_EQUAL(t, bytes.left(4), QByteArray("FORM"));
    AUDEX_EQUAL(t, qFromBigEndian<quint32>(bytes.constData() + 4), quint32(46 + 4000));
    AUDEX_EQUAL(t, bytes.mid(8, 8), QByteArray("AIFFCOMM"));
    AUDEX_EQUAL(t, qFromBigEndian<quint16>(bytes.constData() + 20), quint16(2));
    AUDEX_EQUAL(t, qFromBigEndian<quint32>(bytes.constData() + 22), quint32(1000));
    AUDEX_EQUAL(t, qFromBigEndian<quint16>(bytes.constData() + 26), quint16(16));
    AUDEX_EQUAL(t, bytes.mid(28, 10), QByteArray::fromHex("400EAC44000000000000"));
    AUDEX_EQUAL(t, bytes.mid(38, 4), QByteArray("SSND"));
    AUDEX_EQUAL(t, qFromBigEndian<quint32>(bytes.constData() + 42), quint32(8 + 4000));
    AUDEX_EQUAL(t, qFromBigEndian<qint16>(bytes.constData() + 54), qFromLittleEndian<qint16>(audio.constData()));
    AUDEX_EQUAL(t, qFromBigEndian<qint16>(bytes.constData() + 54 + 3998), qFromLittleEndian<qint16>(audio.constData() + 3998));
    file.close();

    // tags go into an "ID3 " chunk behind the audio; reading back skips it
    TagInfo info;
    info.album.setText(Metadata::Field::Artist, u"The Test Tones"_s);
    info.album.track(1).setText(Metadata::Field::Title, u"440 Hz"_s);
    info.trackNumber = 1;
    info.displayTrackNumber = 1;
    info.trackTotal = 2;
    AUDEX_CHECK_MSG(t, TagWriter::write(path, u"aiff"_s, info, &error), error);
    {
        TagLib::FileRef ref(QFile::encodeName(path).constData());
        AUDEX_CHECK(t, !ref.isNull());
        if (!ref.isNull()) {
            const TagLib::PropertyMap properties = ref.file()->properties();
            AUDEX_EQUAL(t, QString::fromStdString(properties["ARTIST"].toString().to8Bit(true)), u"The Test Tones"_s);
            AUDEX_EQUAL(t, QString::fromStdString(properties["TITLE"].toString().to8Bit(true)), u"440 Hz"_s);
        }
    }

    std::unique_ptr<AudioDecoder> decoder = factory->createDecoder();
    AUDEX_CHECK(t, decoder != nullptr);
    AUDEX_CHECK_MSG(t, decoder->open(path, &error), error);
    AUDEX_EQUAL(t, decoder->totalFrames(), qint64(1000));
    QByteArray decoded;
    QByteArray part;
    do {
        AUDEX_CHECK_MSG(t, decoder->read(part, &error), error);
        decoded += part;
    } while (!part.isEmpty());
    AUDEX_CHECK(t, decoded == audio);
}

AUDEX_TEST("aiff: fewer frames than announced fix the header")
{
    EncoderRegistry registry;
    EncoderFactory *factory = aiffFactory(registry);
    if (!factory)
        return;
    QTemporaryDir dir;
    const QString path = dir.filePath(u"short.aiff"_s);
    QString error;
    std::unique_ptr<AudioEncoder> encoder = factory->create({});
    AUDEX_CHECK(t, encoder->open(path, 1000, &error));
    AUDEX_CHECK(t, encoder->write(pcm(10), &error));
    AUDEX_CHECK(t, encoder->finish(&error));

    std::unique_ptr<AudioDecoder> decoder = factory->createDecoder();
    AUDEX_CHECK_MSG(t, decoder->open(path, &error), error);
    AUDEX_EQUAL(t, decoder->totalFrames(), qint64(10));
}
