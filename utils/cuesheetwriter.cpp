/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cuesheetwriter.h"

CueSheetWriter::CueSheetWriter(CDDAModel *model)
{
    this->model = model;
}

CueSheetWriter::~CueSheetWriter()
{
}

QStringList CueSheetWriter::cueSheet(const QString &binFilename) const
{
    QStringList result;

    result << "REM cue file written by Audex Version " AUDEX_VERSION;
    result << QString("REM GENRE \"%1\"").arg(model->genre());
    result << QString("REM DATE \"%1\"").arg(model->year());
    result << QString("PERFORMER \"%1\"").arg(model->artist());
    result << QString("TITLE \"%1\"").arg(model->title());

    QFileInfo info(binFilename);
    result << QString("FILE \"%1\" %2").arg(info.fileName(), p_filetype(binFilename));

    const QList<unsigned> dsig = model->discSignature();

    int leadin = 0;
    for (int i = 0; i < dsig.count() - 1; ++i) {
        if (!model->isAudioTrack(i + 1))
            continue;
        result << QString("  TRACK %1 AUDIO").arg(i + 1, 2, 10, QChar('0'));
        result << QString("    PERFORMER \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString());
        result << QString("    TITLE \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString());
        if ((i == 0) && (dsig.at(i) > 0))
            leadin = dsig.at(i);
        float l = (float)(dsig.at(i) - leadin) / 75.0f;
        int min = (int)l / 60;
        int sec = (int)l % 60;
        int frames = (dsig.at(i) - leadin) - (((min * 60) + sec) * 75);
        result << QString("    INDEX 01 %1:%2:%3").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(frames, 2, 10, QChar('0'));
    }

    return result;
}

QStringList CueSheetWriter::cueSheet(const QStringList &filenames) const
{
    QStringList result;
    result << "REM cue file written by Audex Version " AUDEX_VERSION;
    result << QString("REM GENRE \"%1\"").arg(model->genre());
    result << QString("REM DATE \"%1\"").arg(model->year());
    result << QString("PERFORMER \"%1\"").arg(model->artist());
    result << QString("TITLE \"%1\"").arg(model->title());

    for (int i = 0; i < filenames.count(); ++i) {
        QFileInfo info(filenames.at(i));
        result << QString("FILE \"%1\" %2").arg(info.fileName(), p_filetype(filenames.at(i)));

        result << QString("  TRACK %1 AUDIO").arg(i + 1, 2, 10, QChar('0'));
        result << QString("    PERFORMER \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString());
        result << QString("    TITLE \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString());
        result << QString("    INDEX 01 00:00:00");
    }

    return result;
}

QString CueSheetWriter::p_filetype(const QString &filename) const
{
    QString result = "WAVE";
    if ((filename.endsWith(QLatin1String("aiff"), Qt::CaseInsensitive)) || (filename.endsWith(QLatin1String("aif"), Qt::CaseInsensitive))) {
        result = "AIFF";
    } else if (filename.endsWith(QLatin1String("mp3"), Qt::CaseInsensitive)) {
        result = "MP3";
    }

    return result;
}
