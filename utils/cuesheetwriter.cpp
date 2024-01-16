/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
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

QStringList CueSheetWriter::cueSheet(const QString &binFilename, const int frameOffset, const bool writeMCN, const bool writeISRC) const
{
    QStringList result;

    result << "REM cue file written by Audex Version " AUDEX_VERSION;
    result << QString("REM DISCID %1").arg(DiscIDCalculator::CDDBId(model->discSignature()), 8, 16, QLatin1Char('g')).toUpper();
    result << QString("REM GENRE \"%1\"").arg(model->genre());
    result << QString("REM DATE \"%1\"").arg(model->year());
    if (writeMCN) {
        QString mcn = model->cdio()->getMCN();
        if (!mcn.isEmpty() && mcn != "0")
            result << QString("CATALOG %1").arg(mcn);
    }
    result << QString("PERFORMER \"%1\"").arg(model->artist());
    result << QString("TITLE \"%1\"").arg(model->title());

    QFileInfo info(binFilename);
    result << QString("FILE \"%1\" %2").arg(info.fileName(), p_filetype(binFilename));

    for (int i = 0; i < model->cdio()->numOfTracks(); ++i) {
        if (!model->isAudioTrack(i + 1))
            continue;
        result << QString("  TRACK %1 AUDIO").arg(i + 1, 2, 10, QChar('0'));
        if (writeISRC) {
            QString isrc = model->cdio()->getISRC(i + 1);
            if (!isrc.isEmpty() && isrc != "0")
                result << QString("    ISRC %1").arg(isrc);
        }
        result << QString("    PERFORMER \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString());
        result << QString("    TITLE \"%1\"").arg(model->data(model->index(i, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString());

        if (i == 0 && model->cdio()->firstSectorOfDisc() < model->cdio()->firstSectorOfTrack(1) + frameOffset) {
            result << QString("    INDEX 00 %1").arg(CDDACDIO::LSN2MSF(model->cdio()->firstSectorOfDisc()));
        }

        result << QString("    INDEX 01 %1").arg(model->cdio()->msfOfTrack(i + 1));
    }

    return result;
}

QStringList CueSheetWriter::cueSheet(const QStringList &filenames, const int frameOffset, const bool writeMCN, const bool writeISRC) const
{
    Q_UNUSED(frameOffset);

    QStringList result;
    result << "REM cue file written by Audex Version " AUDEX_VERSION;
    result << QString("REM DISCID %1").arg(DiscIDCalculator::CDDBId(model->discSignature()), 8, 16, QLatin1Char('g')).toUpper();
    result << QString("REM GENRE \"%1\"").arg(model->genre());
    result << QString("REM DATE \"%1\"").arg(model->year());
    if (writeMCN) {
        QString mcn = model->cdio()->getMCN();
        if (!mcn.isEmpty() && mcn != "0")
            result << QString("CATALOG %1").arg(mcn);
    }
    result << QString("PERFORMER \"%1\"").arg(model->artist());
    result << QString("TITLE \"%1\"").arg(model->title());

    for (int i = 0; i < filenames.count(); ++i) {
        QFileInfo info(filenames.at(i));
        result << QString("FILE \"%1\" %2").arg(info.fileName(), p_filetype(filenames.at(i)));

        result << QString("  TRACK %1 AUDIO").arg(i + 1, 2, 10, QChar('0'));
        if (writeISRC) {
            QString isrc = model->cdio()->getISRC(i + 1);
            if (!isrc.isEmpty() && isrc != "0")
                result << QString("    ISRC %1").arg(isrc);
        }
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
