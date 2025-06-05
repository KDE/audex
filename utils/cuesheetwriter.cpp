/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cuesheetwriter.h"

#include "discidcalculator.h"

#include "audex-version.h"

namespace Audex
{

CUESheetWriter::CUESheetWriter(const CDDA &cdda)
{
    this->cdda = cdda;
}

CUESheetWriter::~CUESheetWriter()
{
}

QStringList CUESheetWriter::cueSheet(const QString &binFilename, const int frameOffset, const bool writeMCN, const bool writeISRC) const
{
    QStringList result;

    result << QStringLiteral("REM cue file written by Audex Version %1").arg(AUDEX_VERSION_STRING);
    result << QStringLiteral("REM DISCID %1").arg(DiscIDCalculator::CDDBId(cdda.toc().discSignature()), 8, 16, QChar(u'0'));
    QString genre = cdda.metadata().get(Metadata::Genre).toString();
    if (!genre.isEmpty())
        result << QStringLiteral(u"REM GENRE \"%1\"").arg(genre);
    QString date = cdda.metadata().get(Metadata::Year).toString();
    if (!date.isEmpty())
        result << QStringLiteral(u"REM DATE \"%1\"").arg(date);
    if (writeMCN) {
        QString mcn = QString::fromLatin1(cdda.metadata().get(Metadata::MCN).toByteArray());
        if (!mcn.isEmpty() && mcn != QStringLiteral(u"0"))
            result << QStringLiteral("CATALOG %1").arg(mcn);
    }
    QString performer = cdda.metadata().get(Metadata::Artist).toString();
    if (!performer.isEmpty())
        result << QStringLiteral(u"PERFORMER \"%1\"").arg(performer);
    QString title = cdda.metadata().get(Metadata::Album).toString();
    if (!title.isEmpty())
        result << QStringLiteral(u"TITLE \"%1\"").arg(title);

    QFileInfo info(binFilename);
    result << QStringLiteral(u"FILE \"%1\" %2").arg(info.fileName(), filetype(binFilename));

    for (int i = 1; i <= cdda.toc().trackCount(); ++i) {
        if (!cdda.toc().isAudioTrack(i))
            continue;
        result << QStringLiteral("  TRACK %1 AUDIO").arg(i, 2, 10, QChar(u'0'));
        if (writeISRC) {
            QString isrc = QString::fromLatin1(cdda.metadata().track(i).get(Metadata::ISRC).toByteArray());
            if (!isrc.isEmpty() && isrc != QStringLiteral(u"0"))
                result << QStringLiteral("    ISRC %1").arg(isrc);
        }
        result << QStringLiteral(u"    PERFORMER \"%1\"").arg(cdda.metadata().track(i).get(Metadata::Artist).toString());
        result << QStringLiteral(u"    TITLE \"%1\"").arg(cdda.metadata().track(i).get(Metadata::Title).toString());

        if (i == 0 && cdda.toc().firstSectorOfDisc() < cdda.toc().firstSectorOfTrack(1) + frameOffset) {
            result << QString("    INDEX 00 %1").arg(Toc::Frames2MSFString(cdda.toc().firstSectorOfDisc()));
        }

        result << QString("    INDEX 01 %1").arg(Toc::Frames2MSFString(cdda.toc().firstSectorOfTrack(i)));
    }

    return result;
}

QStringList CUESheetWriter::cueSheet(const QStringList &filenames, const int frameOffset, const bool writeMCN, const bool writeISRC) const
{
    Q_UNUSED(frameOffset);

    QStringList result;
    result << QStringLiteral("REM cue file written by Audex Version %1").arg(AUDEX_VERSION_STRING);
    result << QString("REM DISCID %1").arg(DiscIDCalculator::CDDBId(cdda.toc().discSignature()), 8, 16, QLatin1Char('g')).toUpper();
    QString genre = cdda.metadata().get(Metadata::Genre).toString();
    if (!genre.isEmpty())
        result << QStringLiteral(u"REM GENRE \"%1\"").arg(genre);
    QString date = cdda.metadata().get(Metadata::Year).toString();
    if (!date.isEmpty())
        result << QStringLiteral(u"REM DATE \"%1\"").arg(date);
    if (writeMCN) {
        QString mcn = QString::fromLatin1(cdda.metadata().get(Metadata::MCN).toByteArray());
        if (!mcn.isEmpty() && mcn != QStringLiteral(u"0"))
            result << QStringLiteral("CATALOG %1").arg(mcn);
    }
    QString performer = cdda.metadata().get(Metadata::Artist).toString();
    if (!performer.isEmpty())
        result << QStringLiteral(u"PERFORMER \"%1\"").arg(performer);
    QString title = cdda.metadata().get(Metadata::Album).toString();
    if (!title.isEmpty())
        result << QStringLiteral(u"TITLE \"%1\"").arg(title);

    for (int i = 0; i < filenames.count(); ++i) {
        QFileInfo info(filenames.at(i));
        result << QString(u"FILE \"%1\" %2").arg(info.fileName(), filetype(filenames.at(i)));

        result << QString("  TRACK %1 AUDIO").arg(i + 1, 2, 10, QChar('0'));
        if (writeISRC) {
            QString isrc = QString::fromLatin1(cdda.metadata().track(i + 1).get(Metadata::ISRC).toByteArray());
            if (!isrc.isEmpty() && isrc != QStringLiteral(u"0"))
                result << QStringLiteral("    ISRC %1").arg(isrc);
        }
        result << QStringLiteral(u"    PERFORMER \"%1\"").arg(cdda.metadata().track(i + 1).get(Metadata::Artist).toString());
        result << QStringLiteral(u"    TITLE \"%1\"").arg(cdda.metadata().track(i + 1).get(Metadata::Title).toString());
        result << QStringLiteral("    INDEX 01 00:00:00");
    }

    return result;
}

QString CUESheetWriter::filetype(const QString &filename) const
{
    QString result = QStringLiteral(u"WAVE");
    if ((filename.endsWith(QStringLiteral("aiff"), Qt::CaseInsensitive)) || (filename.endsWith(QStringLiteral("aif"), Qt::CaseInsensitive))) {
        result = QStringLiteral("AIFF");
    } else if (filename.endsWith(QLatin1String("mp3"), Qt::CaseInsensitive)) {
        result = QStringLiteral("MP3");
    }

    return result;
}

}
