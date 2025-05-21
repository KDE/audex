/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "toc.h"

namespace Audex
{

namespace Toc
{

const Track &Toc::track(const int tracknumber) const
{
    return subset(tracknumber - 1);
}

Track &Toc::track(const int tracknumber)
{
    return subset(tracknumber - 1);
}

const Track &Toc::firstTrack() const
{
    return firstSubset();
}

Track &Toc::firstTrack()
{
    return firstSubset();
}

const Track &Toc::lastTrack() const
{
    return lastSubset();
}

Track &Toc::lastTrack()
{
    return lastSubset();
}

int Toc::appendTrack(const Track &track)
{
    return appendSubset(track) + 1;
}

int Toc::trackCount() const
{
    return subsetCount();
}

int Toc::firstTracknumber() const
{
    return 1;
}

int Toc::lastTracknumber() const
{
    return subsetCount();
}

int Toc::firstSectorOfDisc(const bool leadin) const
{
    return (leadin ? get(FirstSector, 0).toInt() : get(FirstSector, 0).toInt() - LEADIN_SECTOR_COUNT);
}

int Toc::lastSectorOfDisc(const bool leadin) const
{
    return (leadin ? get(LastSector, 0).toInt() : get(LastSector, 0).toInt() - LEADIN_SECTOR_COUNT);
}

int Toc::firstSectorOfTrack(const int tracknumber, const bool leadin) const
{
    if (tracknumber < firstTracknumber() || tracknumber > lastTracknumber())
        return 0;
    return (leadin ? track(tracknumber).get(FirstSector, 0).toInt() : track(tracknumber).get(FirstSector, 0).toInt() - LEADIN_SECTOR_COUNT);
}

int Toc::lastSectorOfTrack(const int tracknumber, const bool leadin) const
{
    if (tracknumber < firstTracknumber() || tracknumber > lastTracknumber())
        return 0;
    return (leadin ? track(tracknumber).get(LastSector, 0).toInt() : track(tracknumber).get(LastSector, 0).toInt() - LEADIN_SECTOR_COUNT);
}

int Toc::lastSectorOfLastAudioTrack(const bool leadin) const
{
    int t = 1;
    while (t < lastTracknumber() && track(t).get(Format).value<TrackFormat>() == AUDIO)
        ++t;
    return lastSectorOfTrack(t, leadin);
}

int Toc::audioTrackCount() const
{
    int c = 0;
    for (int t = firstTracknumber(); t <= lastTracknumber(); ++t)
        if (track(t).get(Format).value<TrackFormat>() == AUDIO)
            ++c;
    return c;
}

int Toc::lengthOfAudioTracks() const
{
    return sectorCountAudioTracks() / SECTORS_PER_SECOND;
}

int Toc::sectorCountAudioTracks() const
{
    int c = 0;
    for (int t = firstTracknumber(); t <= lastTracknumber(); ++t)
        if (track(t).get(Format).value<TrackFormat>() == AUDIO)
            c += sectorCountTrack(t);
    return c;
}

int Toc::lengthOfTrack(const int track) const
{
    return sectorCountTrack(track) / SECTORS_PER_SECOND;
}

int Toc::lengthOfTracknumbers(const TracknumberSet &setOfTracknumbers) const
{
    int l = 0;
    for (auto i = setOfTracknumbers.cbegin(), end = setOfTracknumbers.cend(); i != end; ++i)
        l += lengthOfTrack(*i);
    return l;
}

int Toc::sectorCountTrack(const int tracknumber) const
{
    if (tracknumber < firstTracknumber() || tracknumber > lastTracknumber())
        return 0;
    return lastSectorOfTrack(tracknumber) - firstSectorOfTrack(tracknumber) + 1;
}

int Toc::sectorCountTracknumbers(const TracknumberSet &setOfTracknumbers, const bool onlyAudioTracks) const
{
    int c = 0;
    for (auto i = setOfTracknumbers.cbegin(), end = setOfTracknumbers.cend(); i != end; ++i) {
        if (!onlyAudioTracks || isAudioTrack(*i))
            c += sectorCountTrack(*i);
    }
    return c;
}

qreal Toc::sizeTrack(const int tracknumber) const
{
    if (trackCount() > 0) {
        auto sector_size = (qreal)(sectorCountTrack(tracknumber));
        if (isAudioTrack(tracknumber))
            return (sector_size * 2352.0f) / (1024.0f * 1024.0f);
        else if (track(tracknumber).get(Format).value<TrackFormat>() == DATA && track(tracknumber).get(DataMode).value<TrackDataMode>() == MODE1)
            return (sector_size * 2048.0f) / (1024.0f * 1024.0f);
        else if (track(tracknumber).get(Format).value<TrackFormat>() == DATA && track(tracknumber).get(DataMode).value<TrackDataMode>() == MODE2)
            return (sector_size * 2336.0f) / (1024.0f * 1024.0f);
    }
    return 0.0f;
}

bool Toc::nullTrackAvailable() const
{
    return sectorCountOfNullTrack() > 0;
}

int Toc::firstSectorOfNullTrack(const bool leadin) const
{
    if (trackCount() == 0)
        return -1;
    if (firstSectorOfTrack(firstTracknumber()) > LEADIN_SECTOR_COUNT + 1) {
        return leadin ? LEADIN_SECTOR_COUNT + 1 : 1;
    }
    return -1;
}

int Toc::lastSectorOfNullTrack(const bool leadin) const
{
    if (trackCount() == 0)
        return -1;
    if (firstSectorOfTrack(firstTracknumber()) > LEADIN_SECTOR_COUNT + 1) {
        return firstSectorOfTrack(firstTracknumber(), leadin) - 1;
    }
    return -1;
}

int Toc::sectorCountOfNullTrack() const
{
    return lastSectorOfNullTrack() - firstSectorOfNullTrack();
}

int Toc::lengthOfNullTrack() const
{
    return sectorCountOfNullTrack() / SECTORS_PER_SECOND;
}

bool Toc::isAudioTrack(const int tracknumber) const
{
    if (tracknumber == 0 && nullTrackAvailable())
        return track(firstTracknumber()).get(Format).value<TrackFormat>() == AUDIO;
    if (tracknumber < firstTracknumber() || tracknumber > lastTracknumber())
        return false;
    return track(tracknumber).get(Format).value<TrackFormat>() == AUDIO;
}

bool Toc::isDataTrack(const int tracknumber) const
{
    if (tracknumber == 0 && nullTrackAvailable())
        return track(firstTracknumber()).get(Format).value<TrackFormat>() == DATA;
    if (tracknumber < firstTracknumber() || tracknumber > lastTracknumber())
        return false;
    return track(tracknumber).get(Format).value<TrackFormat>() == DATA;
}

bool Toc::isFirstTrack(const int tracknumber) const
{
    return tracknumber == firstTracknumber();
}

bool Toc::isLastTrack(const int tracknumber) const
{
    return tracknumber == lastTracknumber();
}

bool Toc::isAudioDisc() const
{
    for (int t = firstTracknumber(); t <= lastTracknumber(); ++t)
        if (isAudioTrack(t))
            return true;
    return false;
}

bool Toc::isPureAudioDisc() const
{
    for (int t = firstTracknumber(); t <= lastTracknumber(); ++t)
        if (!isAudioTrack(t))
            return false;
    return true;
}

TrackOffsetList Toc::trackOffsetList(const bool leadin, const bool skipDataTracks) const
{
    TrackOffsetList result;
    for (int t = firstTracknumber(); t <= lastTracknumber(); ++t) {
        if (skipDataTracks && !isAudioTrack(t))
            continue;
        result.append((quint32)firstSectorOfTrack(t, leadin));
    }
    result.append((quint32)lastSectorOfDisc(leadin) + 1);
    return result;
}

const QStringList Toc::prettyTOC() const
{
    QStringList result;

    if (trackCount() == 0)
        return result;

    for (int t = firstTracknumber(); t <= lastTracknumber(); ++t) {
        QString tracknumber = QStringLiteral(u"%1").arg(t, 2, 10, QChar(u'0'));

        QString start = QStringLiteral(u"%1").arg(this->track(t).get(FirstSector, 0).toInt(), 6, 10, QChar(u'0'));

        QString size;
        int size_i = this->track(t).get(LastSector, 0).toInt() - this->track(t).get(FirstSector, 0).toInt() + 1;
        if (size_i > -1)
            size = QStringLiteral(u"%1").arg(size_i, 6, 10, QChar(u'0'));
        else
            size = QStringLiteral(u"------");

        QString type;
        switch (this->track(t).get(Format).value<TrackFormat>()) {
        case AUDIO:
            type = i18n("Audio");
            break;
        case DATA:
            if (this->track(t).get(DataMode).value<TrackDataMode>() == MODE1)
                type = i18n("Data Mode 1");
            else if (this->track(t).get(DataMode).value<TrackDataMode>() == MODE2)
                type = i18n("Data Mode 2");
            else
                type = i18n("Data");
            break;
        case UNKNOWN_FORMAT:
            type = i18n("Non-Audio: Unknown format");
            break;
        }

        quint32 crc32 = this->track(t).get(ChecksumCRC32).toUInt();

        QString line = i18n("Track %1, Start sector: %2, Size in sectors: %3, Type: %4", tracknumber, start, size, type);
        if (crc32 > 0)
            line += i18n(", CRC32: %1", QStringLiteral(u"%1").arg(crc32, 8, 16, QChar(u'0')));

        result.append(line);
    }

    return result;
}

bool Toc::trackPreEmphasis(const int tracknumber) const
{
    if (tracknumber < firstTracknumber() || tracknumber > lastTracknumber())
        return false;
    return track(tracknumber).get(PreEmphasis, false).toBool();
}

bool Toc::trackCopyPermitted(const int tracknumber) const
{
    if (tracknumber < firstTracknumber() || tracknumber > lastTracknumber())
        return false;
    return track(tracknumber).get(CopyPermitted, false).toBool();
}

const QString Toc::msfStringOfTrack(const int track, const bool leadin) const
{
    int sectors = firstSectorOfTrack(track);
    if (leadin)
        sectors += LEADIN_SECTOR_COUNT;
    return Frames2MSFString(sectors);
}

void Toc::setFirstSectorOfDisc(const int first_sector_of_disc)
{
    set(FirstSector, first_sector_of_disc);
}

void Toc::setLastSectorOfDisc(const int last_sector_of_disc)
{
    set(LastSector, last_sector_of_disc);
}

const QString Frames2MSFString(const int lsn, const QChar sectorSeparator)
{
    qreal length = (qreal)(lsn) / (qreal)SECTORS_PER_SECOND;
    int min = (int)length / 60;
    int sec = (int)length % 60;
    int sectors = lsn - (((min * 60) + sec) * SECTORS_PER_SECOND);
    return QStringLiteral(u"%1:%2%3%4").arg(min, 2, 10, QChar(u'0')).arg(sec, 2, 10, QChar(u'0')).arg(sectorSeparator).arg(sectors, 2, 10, QChar(u'0'));
}

int MSF2Frames(const int min, const int sec, const int frames)
{
    return (min * 60 + sec) * SECTORS_PER_SECOND + frames;
}

const QString Frames2TimeString(const int lsn, const QChar separator)
{
    qreal length = (qreal)(lsn) / (qreal)SECTORS_PER_SECOND;
    int min = (int)length / 60;
    int sec = (int)length % 60;
    int msec = (int)((lsn - (((min * 60) + sec) * SECTORS_PER_SECOND)) * (40.0f / 3.0f));
    return QStringLiteral(u"%1").arg(min, 2, 10, QChar(u'0')) + separator + QStringLiteral(u"%1").arg(sec, 2, 10, QChar(u'0')) + separator + separator
        + QStringLiteral(u"%1").arg(msec, 3, 10, QChar(u'0'));
}

QDebug operator<<(QDebug debug, const Toc &toc)
{
    QDebugStateSaver saver(debug);

    debug.nospace().noquote() << "First track number: " << toc.firstTracknumber() << Qt::endl;
    debug.nospace().noquote() << "Last track number: " << toc.lastTracknumber() << Qt::endl;

    QStringList stoc = toc.prettyTOC();

    for (int i = 0; i < stoc.count(); ++i)
        debug.nospace().noquote() << stoc.at(i) << Qt::endl;

    return debug;
}

}

}
