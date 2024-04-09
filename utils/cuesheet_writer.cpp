/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cuesheet_writer.h"

namespace Audex
{

CUESheetWriter::CUESheetWriter(const CDInfo &cdInfo)
{
    this->cdinfo = cdInfo;
}

CUESheetWriter::~CUESheetWriter()
{
}

QStringList CUESheetWriter::cueSheet(const QString &binFilename, const int frameOffset, const bool writeMCN, const bool writeISRC) const
{
    QStringList result;

    result << QStringLiteral(u"REM cue file written by Audex Version AUDEX_VERSION_STRING"); //TODO
    result << QStringLiteral("REM DISCID %1").arg(CDDBId(cdinfo.toc().trackOffsetList()), 8, 16, QChar(u'0'));
    QString genre = cdinfo.metadata().get(Metadata::Genre).toString();
    if (!genre.isEmpty())
        result << QStringLiteral(u"REM GENRE \"%1\"").arg(genre);
    QString date = cdinfo.metadata().get(Metadata::Year).toString();
    if (!date.isEmpty())
        result << QStringLiteral(u"REM DATE \"%1\"").arg(date);
    if (writeMCN) {
        QString mcn = QString::fromLatin1(cdinfo.metadata().get(Metadata::MCN).toByteArray());
        if (!mcn.isEmpty() && mcn != QStringLiteral(u"0"))
            result << QStringLiteral("CATALOG %1").arg(mcn);
    }
    QString performer = cdinfo.metadata().get(Metadata::Artist).toString();
    if (!performer.isEmpty())
        result << QStringLiteral("PERFORMER \"%1\"").arg(performer);
    QString title = cdinfo.metadata().get(Metadata::Album).toString();
    if (!title.isEmpty())
        result << QStringLiteral("TITLE \"%1\"").arg(title);

    QFileInfo info(binFilename);
    result << QStringLiteral("FILE \"%1\" %2").arg(info.fileName(), filetype(binFilename));

    for (int i = 0; i < cdinfo.toc().trackCount(); ++i) {
        if (!cdinfo.toc().isAudioTrack(i + 1))
            continue;
        result << QStringLiteral("  TRACK %1 AUDIO").arg(i + 1, 2, 10, QChar(u'0'));
        if (writeISRC) {
            QString isrc = QString::fromLatin1(cdinfo.metadata().track(i+1).get(Metadata::ISRC).toByteArray());
            if (!isrc.isEmpty() && isrc != QStringLiteral(u"0"))
                result << QStringLiteral(u"    ISRC %1").arg(isrc);
        }
        result << QStringLiteral(u"    PERFORMER \"%1\"").arg(cdinfo.metadata().track(i+1).get(Metadata::Artist).toString());
        result << QStringLiteral(u"    TITLE \"%1\"").arg(cdinfo.metadata().track(i+1).get(Metadata::Title).toString());

        result << QStringLiteral(u"    INDEX 01 %1").arg(Toc::Frames2MSFString(cdinfo.toc().firstSectorOfTrack(i+1)));
    }

    return result;
}

QString CUESheetWriter::filetype(const QString &filename) const
{
    QString result = QStringLiteral(u"WAVE");
    if ((filename.endsWith(QStringLiteral(u"aiff"), Qt::CaseInsensitive)) || (filename.endsWith(QStringLiteral(u"aif"), Qt::CaseInsensitive))) {
        result = QStringLiteral(u"AIFF");
    } else if (filename.endsWith(QLatin1String("mp3"), Qt::CaseInsensitive)) {
        result = QStringLiteral(u"MP3");
    }

    return result;
}

}
