/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaparanoia.h"

#include <QDebug>
#include <cdio/track.h>

/* some of this code in here is based on k3b 0.8.x sourcecode */

CDDAParanoia::CDDAParanoia(QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(parent);
    paranoia = nullptr;
    drive = nullptr;
    enableNeverSkip();
    setMaxRetries(20);
    enableFullParanoiaMode();
}

CDDAParanoia::~CDDAParanoia()
{
    _paranoia_free();
}

bool CDDAParanoia::setDevice(const QString &device)
{
    if ((device.isEmpty() && (_device.isEmpty())))
        _device = "/dev/cdrom";
    if (!device.isEmpty())
        _device = device;
    if (!_paranoia_init()) {
        qDebug() << "Internal device error.";
        Q_EMIT error(i18n("Internal device error."), i18n("Check your device. Is it really \"%1\"? If so also check your permissions on \"%1\".", _device));
        return false;
    }
    return true;
}

QString CDDAParanoia::device() const
{
    return _device;
}

void CDDAParanoia::enableFullParanoiaMode(const bool enabled)
{
    mutex.lock();

    if (enabled)
        paranoia_mode = PARANOIA_MODE_FULL;
    else
        paranoia_mode = PARANOIA_MODE_DISABLE;

    if (paranoia_never_skip)
        paranoia_mode |= PARANOIA_MODE_NEVERSKIP;
    else
        paranoia_mode &= ~PARANOIA_MODE_NEVERSKIP;

    if (paranoia)
        paranoia_modeset(paranoia, paranoia_mode);

    mutex.unlock();
}

void CDDAParanoia::enableNeverSkip(const bool never_skip)
{
    paranoia_never_skip = never_skip;
    if (paranoia_never_skip)
        paranoia_mode |= PARANOIA_MODE_NEVERSKIP;
    else
        paranoia_mode &= ~PARANOIA_MODE_NEVERSKIP;

    if (paranoia)
        paranoia_modeset(paranoia, paranoia_mode);
}

void CDDAParanoia::setMaxRetries(int m)
{
    paranoia_max_retries = m;
}

qint16 *CDDAParanoia::paranoiaRead(void (*callback)(long, paranoia_cb_mode_t))
{
    if (paranoia) {
        mutex.lock();
        int16_t *data = cdio_paranoia_read_limited(paranoia, callback, paranoia_max_retries);
        mutex.unlock();
        return data;
    }
    return nullptr;
}

int CDDAParanoia::paranoiaSeek(long sector, qint32 mode)
{
    if (paranoia) {
        mutex.lock();
        long pos = cdio_paranoia_seek(paranoia, sector, mode);
        mutex.unlock();
        return pos;
    }
    return -1;
}

int CDDAParanoia::firstSectorOfTrack(track_t track)
{
    if (drive) {
        mutex.lock();
        long first_sector = cdio_cddap_track_firstsector(drive, track);
        mutex.unlock();
        return first_sector;
    }
    return -1;
}

int CDDAParanoia::lastSectorOfTrack(track_t track)
{
    if (drive) {
        mutex.lock();
        long last_sector = cdio_cddap_track_lastsector(drive, track);
        mutex.unlock();
        return last_sector;
    }
    return -1;
}

int CDDAParanoia::firstSectorOfDisc()
{
    if (drive) {
        mutex.lock();
        long first_sector = cdio_cddap_disc_firstsector(drive);
        mutex.unlock();
        return first_sector;
    }
    return -1;
}

int CDDAParanoia::lastSectorOfDisc()
{
    if (drive) {
        mutex.lock();
        long last_sector = cdio_cddap_disc_lastsector(drive);
        mutex.unlock();
        return last_sector;
    }
    return -1;
}

void CDDAParanoia::sampleOffset(const int offset)
{
    int sample_offset = offset;

    // Hack from cdda paranoia
    if (drive) {
        mutex.lock();

        int toc_offset = 0;
        toc_offset += sample_offset / 588;
        sample_offset %= 588;
        if (sample_offset < 0) {
            sample_offset += 588;
            toc_offset--;
        }

        for (int i = 0; i < drive->tracks + 1; ++i)
            drive->disc_toc[i].dwStartSector += toc_offset;

        mutex.unlock();
    }
}

track_t CDDAParanoia::numOfTracks() const
{
    if (drive)
        return cdio_cddap_tracks(drive);
    return 0;
}

track_t CDDAParanoia::numOfAudioTracks() const
{
    if (numOfTracks() > 0) {
        int result = 0;
        for (track_t t = 1; t <= numOfTracks(); t++)
            if (isAudioTrack(t))
                ++result;
        return result;
    }
    return 0;
}

int CDDAParanoia::length() const
{
    return numOfFrames() / FRAMES_PER_SECOND;
}

int CDDAParanoia::numOfFrames() const
{
    if (numOfTracks() > 0) {
        if (drive)
            return cdio_cddap_disc_lastsector(drive);
    }
    return 0;
}

int CDDAParanoia::lengthOfAudioTracks() const
{
    return numOfFramesOfAudioTracks() / FRAMES_PER_SECOND;
}

int CDDAParanoia::numOfFramesOfAudioTracks() const
{
    if (numOfTracks() > 0) {
        int frames = 0;
        for (track_t t = 1; t <= numOfTracks(); ++t) {
            if (isAudioTrack(t))
                frames += numOfFramesOfTrack(t);
        }
        return frames;
    }
    return 0;
}

int CDDAParanoia::numOfSkippedFrames(int n) const
{
    if (numOfTracks() > 0) {
        if (n < 1)
            n = 1;
        if (n > numOfTracks())
            n = numOfTracks();
        int frames = 0;
        for (int t = 1; t < n; ++t) {
            if (!isAudioTrack(t))
                frames += numOfFramesOfTrack(t);
        }
        return frames;
    }
    return 0;
}

int CDDAParanoia::lengthOfTrack(track_t track) const
{
    if (numOfTracks() > 0) {
        return numOfFramesOfTrack(track) / FRAMES_PER_SECOND;
    }
    return 0;
}

int CDDAParanoia::numOfFramesOfTrack(track_t track) const
{
    if (numOfTracks() > 0) {
        if (track < 1)
            track = 1;
        if (track > numOfTracks())
            track = numOfTracks();

        if (drive)
            return cdio_cddap_track_lastsector(drive, track) - cdio_cddap_track_firstsector(drive, track);
    }
    return 0;
}

double CDDAParanoia::sizeOfTrack(track_t track) const
{
    if (numOfTracks() > 0) {
        auto frame_size = (double)(numOfFramesOfTrack(track));
        if (isAudioTrack(track)) {
            return (frame_size * 2352.0f) / (1024.0f * 1024.0f);
        } else {
            return (frame_size * 2048.0f) / (1024.0f * 1024.0f);
        }
    }
    return 0.0f;
}

int CDDAParanoia::frameOffsetOfTrack(track_t track) const
{
    if (numOfTracks() > 0 && drive)
        return cdio_cddap_track_firstsector(drive, track);
    return 0;
}

bool CDDAParanoia::isAudioTrack(track_t track) const
{
    if (drive)
        return cdio_cddap_track_audiop(drive, track) == 1;
    return true;
}

QList<quint32> CDDAParanoia::discSignature(const qint32 pregap)
{
    QList<quint32> result;

    for (track_t i = 1; i <= numOfTracks() + 1; ++i)
        result.append(frameOffsetOfTrack(i) + pregap);

    return result;
}

void CDDAParanoia::reset()
{
    _paranoia_init();
}

bool CDDAParanoia::_paranoia_init()
{
    mutex.lock();

    _paranoia_free();

    drive = cdda_identify(_device.toLatin1().data(), 0, nullptr);
    if (drive == nullptr) {
        mutex.unlock();
        qDebug() << "Failed to find device.";
        return false;
    }

    // cdda_cdda_verbose_set(_drive, 1, 1);

    cdda_open(drive);
    paranoia = paranoia_init(drive);
    if (paranoia == nullptr) {
        _paranoia_free();
        mutex.unlock();
        qDebug() << "Failed to init device.";
        return false;
    }

    mutex.unlock();
    return true;
}

void CDDAParanoia::_paranoia_free()
{
    // mutex.lock();
    if (paranoia) {
        paranoia_free(paranoia);
        paranoia = nullptr;
    }
    if (drive) {
        cdda_close(drive);
        drive = nullptr;
    }
    // mutex.unlock();
}
