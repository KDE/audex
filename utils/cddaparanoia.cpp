/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaparanoia.h"

CDDAParanoia::CDDAParanoia(const QString &deviceFile, const bool enableParanoiaMode, const int maxRetriesOnReadError, QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(parent);

    device_file = deviceFile;

    drive = cdda_identify(device_file.toLocal8Bit().constData(), 0, nullptr);
    if (!drive) {
        qDebug() << "Failed to find device.";
        Q_EMIT error(i18n("Device error."), i18n("Check your device on \"%1\".", device_file));
        return;
    }

    switch (cdda_open(drive)) {
    case -2:
    case -3:
    case -4:
    case -5:
        qDebug() << "Failed to open disc (1).";
        Q_EMIT error(i18n("Disc error."), i18n("Unable to open disc. Is there an audio disc in the drive \"%1\".", device_file));
        return;
    case -6:
        qDebug() << "Failed to open disc (2).";
        Q_EMIT error(i18n("Disc error."), i18n("cdparanoia could not find a way to read audio from the drive \"%1\".", device_file));
        return;
    case 0:
        break;
    default:
        qDebug() << "Failed to open disc (3).";
        Q_EMIT error(i18n("Disc error."), i18n("Unspecified drive error on \"%1\".", device_file));
        return;
    }

    paranoia = paranoia_init(drive);
    if (!paranoia) {
        qDebug() << "Failed to create cdparanoia object.";
        Q_EMIT error(i18n("Drive/Disc error."), i18n("Unspecified drive or disc error on \"%1\".", device_file));
        return;
    }

    if (enableParanoiaMode)
        paranoia_mode = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;
    else
        paranoia_mode = PARANOIA_MODE_DISABLE;
    paranoia_modeset(paranoia, paranoia_mode);

    paranoia_max_retries = maxRetriesOnReadError;
}

CDDAParanoia::~CDDAParanoia()
{
    if (paranoia) {
        paranoia_free(paranoia);
        paranoia = nullptr;
    }
    if (drive) {
        cdda_close(drive);
        drive = nullptr;
    }
}

void CDDAParanoia::enableParanoiaMode(const bool enable)
{
    if (enable)
        paranoia_mode = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP; // full paranoia mode, but allow skipping
    else
        paranoia_mode = PARANOIA_MODE_DISABLE;

    if (paranoia)
        paranoia_modeset(paranoia, paranoia_mode);
}

void CDDAParanoia::setParanoiaMaxRetriesOnReadError(int max_retries)
{
    paranoia_max_retries = max_retries;
}

qint16 *CDDAParanoia::paranoiaRead(void (*callback)(long, paranoia_cb_mode_t))
{
    if (paranoia)
        return cdio_paranoia_read_limited(paranoia, callback, paranoia_max_retries);
    return nullptr;
}

int CDDAParanoia::paranoiaSeek(const int sector, qint32 mode)
{
    if (paranoia)
        return (int)cdio_paranoia_seek(paranoia, sector, mode);
    return -1;
}

bool CDDAParanoia::paranoiaError(QString &errorMsg)
{
    errorMsg.clear();
    if (drive) {
        char *msg = cdio_cddap_errors(drive);
        if (msg) {
            errorMsg = QString::fromLocal8Bit(msg, -1);
            cdio_cddap_free_messages(msg);
            return true;
        }
    }
    return false;
}

int CDDAParanoia::firstSectorOfTrack(const int track)
{
    if (drive)
        return (int)cdda_track_firstsector(drive, track);
    return -1;
}

int CDDAParanoia::lastSectorOfTrack(const int track)
{
    if (drive)
        return (int)cdda_track_lastsector(drive, track);
    return -1;
}

int CDDAParanoia::firstSectorOfDisc()
{
    if (drive)
        return (int)cdda_disc_firstsector(drive);
    return -1;
}

int CDDAParanoia::lastSectorOfDisc()
{
    if (drive)
        return (int)cdda_disc_lastsector(drive);
    return -1;
}

int CDDAParanoia::firstTrackNum()
{
    if (drive)
        return (int)cdda_sector_gettrack(drive, cdda_disc_firstsector(drive));
    return -1;
}

int CDDAParanoia::lastTrackNum()
{
    if (drive)
        return (int)cdda_sector_gettrack(drive, cdda_disc_lastsector(drive));
    return -1;
}

int CDDAParanoia::numOfTracks()
{
    if (drive)
        return (int)cdda_tracks(drive);
    return -1;
}

int CDDAParanoia::numOfAudioTracks()
{
    if (numOfTracks() > 0) {
        int num = 0;
        for (int t = firstTrackNum(); t <= lastTrackNum(); ++t)
            if (isAudioTrack(t))
                ++num;
        return num;
    }
    return 0;
}

int CDDAParanoia::length()
{
    return numOfFrames() / FRAMES_PER_SECOND;
}

int CDDAParanoia::numOfFrames()
{
    if (numOfTracks() > 0) {
        if (drive)
            return cdda_disc_lastsector(drive);
    }
    return 0;
}

int CDDAParanoia::lengthOfAudioTracks()
{
    return lastSectorOfDisc() / FRAMES_PER_SECOND;
}

int CDDAParanoia::numOfFramesOfAudioTracks()
{
    if (numOfTracks() > 0) {
        int frames = 0;
        for (int t = firstTrackNum(); t <= lastTrackNum(); ++t)
            if (isAudioTrack(t))
                frames += numOfFramesOfTrack(t);
        return frames;
    }
    return 0;
}

int CDDAParanoia::numOfSkippedFrames(const int n)
{
    if (numOfTracks() > 0) {
        int last = n;
        if (last < 1)
            last = 1;
        if (last > numOfTracks())
            last = numOfTracks();
        int frames = 0;
        for (int t = firstTrackNum(); t < last; ++t)
            if (!isAudioTrack(t))
                frames += numOfFramesOfTrack(t);
        return frames;
    }
    return 0;
}

int CDDAParanoia::lengthOfTrack(const int track)
{
    if (numOfTracks() > 0) {
        return numOfFramesOfTrack(track) / FRAMES_PER_SECOND;
    }
    return 0;
}

int CDDAParanoia::numOfFramesOfTrack(const int track)
{
    if (numOfTracks() > 0) {
        int t = track;
        if (t < 1)
            t = 1;
        if (t > numOfTracks())
            t = numOfTracks();
        return lastSectorOfTrack(t) - firstSectorOfTrack(t);
    }
    return 0;
}

bool CDDAParanoia::isPreemphasis(const int track)
{
    if (drive && isAudioTrack(track))
        return cdio_cddap_track_preemp(drive, track) == 1;
    return false;
}

const QString CDDAParanoia::msfOfTrack(const int track)
{
    return LSN2MSF(firstSectorOfTrack(track));
}

qreal CDDAParanoia::sizeOfTrack(const int track)
{
    if (numOfTracks() > 0) {
        auto frame_size = (qreal)(numOfFramesOfTrack(track));
        if (isAudioTrack(track)) {
            return (frame_size * 2352.0f) / (1024.0f * 1024.0f);
        } else {
            return (frame_size * 2048.0f) / (1024.0f * 1024.0f);
        }
    }
    return 0.0f;
}

int CDDAParanoia::frameOffsetOfTrack(const int track)
{
    return firstSectorOfTrack(track);
}

bool CDDAParanoia::isAudioTrack(const int track)
{
    if (drive)
        return cdda_track_audiop(drive, track) == 1;
    return false;
}

bool CDDAParanoia::isDataTrack(const int track)
{
    if (drive)
        return cdda_track_audiop(drive, track) == 0;
    return true;
}

bool CDDAParanoia::isLastTrack(const int track)
{
    return track == lastTrackNum();
}

bool CDDAParanoia::isAudioDisc()
{
    if (drive) {
        for (track_t t = 1; t <= numOfTracks(); ++t)
            if (cdda_track_audiop(drive, t) != 1)
                return false;
    } else {
        return false;
    }
    return true;
}

QList<quint32> CDDAParanoia::discSignature()
{
    QList<quint32> result;

    if (drive)
        for (track_t t = 1; t <= numOfTracks() + 1; ++t)
            result.append(cdda_track_firstsector(drive, t));

    return result;
}

const QStringList CDDAParanoia::prettyTOC()
{
    QStringList result;

    if (drive) {
        result.append(i18n("Track --, Start sector: 000000, Size in sectors: 000150, Type: Lead-in"));

        for (track_t t = 1; t <= numOfTracks() + 1; ++t) {
            QString track;
            track = QString("%1").arg(t, 2, 10, QChar('0'));
            if (t == numOfTracks() + 1)
                track = "--";

            QString start = QString("%1").arg(cdda_track_firstsector(drive, t), 6, 10, QChar('0'));

            QString size;
            int size_i = cdda_track_lastsector(drive, t) - cdda_track_firstsector(drive, t) + 150;
            if (t == numOfTracks() + 1) {
                if (isAudioDisc())
                    size_i = 6750; // lead out of an audio disc is always 90 seconds (== 6750 sectors) long
                else
                    size_i = -1;
            } else if (t == 0) {
                size_i = 150;
            }
            if (size_i > -1)
                size = QString("%1").arg(size_i, 6, 10, QChar('0'));

            QString type;
            if (cdda_track_audiop(drive, t) == 1) {
                type = i18n("Audio");
            } else {
                type = i18n("Non-Audio");
            }

            if (t == numOfTracks() + 1)
                type = i18n("Lead-out");

            result.append(i18n("Track %1, Start sector: %2, Size in sectors: %3, Type: %4", track, start, size, type));
        }
    }

    return result;
}
