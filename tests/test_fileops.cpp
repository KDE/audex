/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "encoding/encodedoutputs.h"
#include "encoding/registry.h"
#include "utils/fileops.h"

#include <QDir>
#include <QTemporaryDir>

using namespace Audex;
using namespace Qt::StringLiterals;

namespace
{

bool writeFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

}

AUDEX_TEST("replacing an existing file keeps its contents on failure")
{
    QTemporaryDir dir;
    const QString source = dir.filePath(u"replacement.wav"_s);
    const QString target = dir.filePath(u"original.wav"_s);
    AUDEX_CHECK(t, writeFile(source, "replacement"));
    AUDEX_CHECK(t, writeFile(target, "original"));

    AUDEX_CHECK(t, !replaceFile(dir.filePath(u"missing.wav"_s), target));
    AUDEX_EQUAL_DATA(t, readFile(target), "original");

    const QString directory = dir.filePath(u"directory"_s);
    AUDEX_CHECK(t, QDir().mkdir(directory));
    AUDEX_CHECK(t, !replaceFile(source, directory));
    AUDEX_EQUAL_DATA(t, readFile(source), "replacement");
    AUDEX_CHECK(t, QFileInfo(directory).isDir());

    AUDEX_CHECK(t, replaceFile(source, target));
    AUDEX_EQUAL_DATA(t, readFile(target), "replacement");
    AUDEX_CHECK(t, !QFile::exists(source));
}

AUDEX_TEST("backups preserve previous audio after replacement")
{
    QTemporaryDir dir;
    const QString original = dir.filePath(u"original.wav"_s);
    const QString backup = dir.filePath(u"original.unrepaired.wav"_s);
    const QString replacement = dir.filePath(u"original.part.wav"_s);
    AUDEX_CHECK(t, writeFile(original, "old audio"));
    AUDEX_CHECK(t, backupFile(original, backup));
    AUDEX_CHECK(t, writeFile(replacement, "new audio"));
    AUDEX_CHECK(t, replaceFile(replacement, original));
    AUDEX_EQUAL_DATA(t, readFile(original), "new audio");
    AUDEX_EQUAL_DATA(t, readFile(backup), "old audio");
}

AUDEX_TEST("encoding replaces existing output after finishing")
{
    QTemporaryDir dir;
    const QString target = dir.filePath(u"disc.wav"_s);
    const QByteArray audio(16, 'a');
    AUDEX_CHECK(t, writeFile(target, "previous rip"));

    Encoding::WavEncoderFactory factory;
    Encoding::OutputTarget output;
    output.path = target;
    output.frames = audio.size() / Encoding::BytesPerFrame;
    Encoding::EncodedOutputs files(factory, {}, {output}, false);
    AUDEX_CHECK(t, files.write(0, audio));
    AUDEX_CHECK_MSG(t, files.finishAll(), files.error());
    AUDEX_CHECK(t, readFile(target).startsWith("RIFF"));
    AUDEX_CHECK(t, readFile(target).endsWith(audio));
    AUDEX_CHECK(t, !QFile::exists(Encoding::EncodedOutputs::partialPath(target)));
}

AUDEX_TEST("encoding leaves the target alone when replacement fails")
{
    QTemporaryDir dir;
    const QString target = dir.filePath(u"disc.wav"_s);
    AUDEX_CHECK(t, QDir().mkdir(target));

    Encoding::WavEncoderFactory factory;
    Encoding::OutputTarget output;
    output.path = target;
    output.frames = 4;
    Encoding::EncodedOutputs files(factory, {}, {output}, false);
    AUDEX_CHECK(t, files.write(0, QByteArray(16, 'a')));
    AUDEX_CHECK(t, !files.finishAll());
    AUDEX_CHECK(t, QFileInfo(target).isDir());
    AUDEX_CHECK(t, QFile::exists(Encoding::EncodedOutputs::partialPath(target)));
}
