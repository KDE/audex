/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cuesheetwriter.h"

CueSheetWriter::CueSheetWriter(CDDAModel *model, const int sampleOffset)
{
    this->model = model;
    sample_offset = sampleOffset;
}

CueSheetWriter::~CueSheetWriter()
{
}

QStringList CueSheetWriter::cueSheet(const QString &binFilename, const int frameOffset) const
{
    QStringList result;

    result << "REM cue file written by Audex Version " AUDEX_VERSION;
    result << QString("REM DISCID \"%1\"").arg(DiscIDCalculator::CDDBId(model->discSignature()), 8, 16, QLatin1Char('g')).toUpper();
    result << QString("REM GENRE \"%1\"").arg(model->genre());
    result << QString("REM DATE \"%1\"").arg(model->year());
    result << QString("PERFORMER \"%1\"").arg(model->artist());
    result << QString("TITLE \"%1\"").arg(model->title());

    QFileInfo info(binFilename);
    result << QString("FILE \"%1\" %2").arg(info.fileName(), p_filetype(binFilename));

    for (int i = 1; i <= model->cdio()->numOfTracks(); ++i) {
        if (!model->isAudioTrack(i))
            continue;
        result << QString("  TRACK %1 AUDIO").arg(i, 2, 10, QChar('0'));
        result << QString("    PERFORMER \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString());
        result << QString("    TITLE \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString());

        if (i == 1 && model->cdio()->firstSectorOfDisc() < model->cdio()->firstSectorOfTrack(1) + frameOffset) {
            qreal length_index0 = (qreal)(model->cdio()->firstSectorOfDisc()) / (qreal)FRAMES_PER_SECOND;
            int min_index0 = (int)length_index0 / 60;
            int sec_index0 = (int)length_index0 % 60;
            int frames_index0 = (model->cdio()->firstSectorOfDisc()) - (((min_index0 * 60) + sec_index0) * FRAMES_PER_SECOND);
            result
                << QString("    INDEX 00 %1:%2:%3").arg(min_index0, 2, 10, QChar('0')).arg(sec_index0, 2, 10, QChar('0')).arg(frames_index0, 2, 10, QChar('0'));
        }

        qreal length = (qreal)(model->cdio()->firstSectorOfTrack(i) + frameOffset) / (qreal)FRAMES_PER_SECOND;
        int min = (int)length / 60;
        int sec = (int)length % 60;
        int frames = (model->cdio()->firstSectorOfTrack(i) + frameOffset) - (((min * 60) + sec) * FRAMES_PER_SECOND);
        result << QString("    INDEX 01 %1:%2:%3").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(frames, 2, 10, QChar('0'));
    }

    return result;
}

QStringList CueSheetWriter::cueSheet(const QStringList &filenames) const
{
    QStringList result;
    result << "REM cue file written by Audex Version " AUDEX_VERSION;
    result << QString("REM DISCID \"%1\"").arg(DiscIDCalculator::CDDBId(model->discSignature()), 8, 16, QLatin1Char('g')).toUpper();
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
