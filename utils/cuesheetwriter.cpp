/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cuesheetwriter.h"
#include "audex-version.h"

using namespace Audex;
using namespace Qt::StringLiterals;
using Metadata::Field;

namespace
{

// cue sheet time format MM:SS:FF, relative to the start of the file
QString cueMsf(const qint64 sectors)
{
    QString msf = Cdda::sectorsToMsf(sectors);
    msf.replace(msf.lastIndexOf(u'.'), 1, u':');
    return msf;
}

QString quoted(QString text)
{
    text.replace(u'\\', u"\\\\"_s);
    text.replace(u'"', u"\\\""_s);
    return u"\"%1\""_s.arg(text);
}

}

CueSheetWriter::CueSheetWriter(const CDInfo &info)
{
    this->info = info;
}

CueSheetWriter::~CueSheetWriter()
{
}

void CueSheetWriter::setSubchannel(const Audex::Cdda::SubchannelScan &scan)
{
    subchannel = scan;
}

QStringList CueSheetWriter::header(const bool writeMCN) const
{
    QStringList result;
    result << u"REM cue file written by Audex Version "_s + QStringLiteral(AUDEX_VERSION_STRING);
    if (!info.cddbDiscId().isEmpty())
        result << u"REM DISCID %1"_s.arg(info.cddbDiscId().toUpper());
    const QString genre = info.metadata().text(Field::Genre);
    if (!genre.isEmpty())
        result << u"REM GENRE %1"_s.arg(quoted(genre));
    const QString year = info.metadata().text(Field::Year);
    if (!year.isEmpty())
        result << u"REM DATE %1"_s.arg(quoted(year));
    if (writeMCN) {
        QString mcn = info.metadata().text(Field::MCN);
        if (mcn.isEmpty())
            mcn = subchannel.mcn;
        if (!mcn.isEmpty() && mcn != u"0"_s)
            result << u"CATALOG %1"_s.arg(mcn);
    }
    result << u"PERFORMER %1"_s.arg(quoted(info.metadata().text(Field::Artist)));
    result << u"TITLE %1"_s.arg(quoted(info.metadata().text(Field::Album)));
    return result;
}

QStringList CueSheetWriter::trackLines(const int number, const bool writeISRC) const
{
    QStringList result;
    const Metadata::Track &track = info.metadata().track(number);
    if (writeISRC) {
        QString isrc = track.text(Field::ISRC);
        if (isrc.isEmpty())
            isrc = subchannel.isrc.value(number);
        if (!isrc.isEmpty() && isrc != u"0"_s)
            result << u"    ISRC %1"_s.arg(isrc);
    }
    QString artist = track.text(Field::Artist);
    if (artist.isEmpty())
        artist = info.metadata().text(Field::Artist);
    result << u"    PERFORMER %1"_s.arg(quoted(artist));
    result << u"    TITLE %1"_s.arg(quoted(track.text(Field::Title)));
    if (const Cdda::Track *t = info.toc().track(number); t && (t->preEmphasis || t->copyPermitted))
        result << u"    FLAGS%1%2"_s.arg(t->copyPermitted ? u" DCP"_s : QString(), t->preEmphasis ? u" PRE"_s : QString());
    return result;
}

QStringList CueSheetWriter::cueSheet(const QString &binFilename, const QList<int> &tracks, const bool writeMCN, const bool writeISRC) const
{
    QStringList result = header(writeMCN);

    const QFileInfo fileInfo(binFilename);
    result << u"FILE \"%1\" %2"_s.arg(fileInfo.fileName(), p_filetype(binFilename));

    if (tracks.isEmpty())
        return result;

    const auto firstEntry = info.entry(tracks.first());
    const int fileStartLba = firstEntry ? firstEntry->firstLba : 0;

    int position = 1; // cue track numbers are 1-based and continuous
    int htoaLba = -1; // hidden track one audio becomes INDEX 00 of the next track
    for (const int number : tracks) {
        const auto entry = info.entry(number);
        if (!entry || !entry->audio)
            continue;
        if (entry->hidden) {
            htoaLba = entry->firstLba;
            continue;
        }
        result << u"  TRACK %1 AUDIO"_s.arg(position, 2, 10, QLatin1Char('0'));
        result << trackLines(number, writeISRC);
        if (htoaLba >= 0) {
            result << u"    INDEX 00 %1"_s.arg(cueMsf(htoaLba - fileStartLba));
            htoaLba = -1;
        } else if (subchannel.index00.contains(number) && subchannel.index00.value(number) >= fileStartLba) {
            result << u"    INDEX 00 %1"_s.arg(cueMsf(subchannel.index00.value(number) - fileStartLba));
        }
        result << u"    INDEX 01 %1"_s.arg(cueMsf(entry->firstLba - fileStartLba));
        const QList<int> indexes = subchannel.indexes.value(number);
        for (int k = 0; k < indexes.size(); ++k)
            result << u"    INDEX %1 %2"_s.arg(k + 2, 2, 10, QLatin1Char('0')).arg(cueMsf(indexes.at(k) - fileStartLba));
        ++position;
    }

    return result;
}

QStringList CueSheetWriter::cueSheet(const QStringList &filenames, const QList<int> &tracks, const bool writeMCN, const bool writeISRC) const
{
    QStringList result = header(writeMCN);

    int position = 1;
    int previous = -1; // TOC number and first sector of the previous file
    int previousLba = 0;
    for (int i = 0; i < filenames.count() && i < tracks.count(); ++i) {
        const int number = tracks.at(i);
        const auto entry = info.entry(number);
        if (!entry || !entry->audio)
            continue;
        const QFileInfo fileInfo(filenames.at(i));
        const QString file = u"FILE \"%1\" %2"_s.arg(fileInfo.fileName(), p_filetype(filenames.at(i)));
        const QString track = u"  TRACK %1 AUDIO"_s.arg(position, 2, 10, QLatin1Char('0'));
        // the pregap lies at the end of the previous file (gaps appended to the previous track)
        const int index00 = subchannel.index00.value(number, -1);
        if (index00 >= 0 && previous == number - 1) {
            result << track << trackLines(number, writeISRC);
            result << u"    INDEX 00 %1"_s.arg(cueMsf(index00 - previousLba));
            result << file;
        } else {
            result << file << track << trackLines(number, writeISRC);
        }
        result << u"    INDEX 01 00:00:00"_s;
        const QList<int> indexes = subchannel.indexes.value(number);
        for (int k = 0; k < indexes.size(); ++k)
            result << u"    INDEX %1 %2"_s.arg(k + 2, 2, 10, QLatin1Char('0')).arg(cueMsf(indexes.at(k) - entry->firstLba));
        previous = number;
        previousLba = entry->firstLba;
        ++position;
    }

    return result;
}

const QString CueSheetWriter::p_filetype(const QString &filename) const
{
    QString result = u"WAVE"_s;
    if ((filename.endsWith(QLatin1String("aiff"), Qt::CaseInsensitive)) || (filename.endsWith(QLatin1String("aif"), Qt::CaseInsensitive))) {
        result = u"AIFF"_s;
    } else if (filename.endsWith(QLatin1String("mp3"), Qt::CaseInsensitive)) {
        result = u"MP3"_s;
    }

    return result;
}
