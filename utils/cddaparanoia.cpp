/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cddaparanoia.h"

#include <QDebug>

/* some of this code in here is based on k3b 0.8.x sourcecode */

CDDAParanoia::CDDAParanoia(QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(parent);
    paranoia = nullptr;
    paranoia_drive = nullptr;
    setNeverSkip(true);
    setMaxRetries(20);
    setParanoiaMode(3);
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

void CDDAParanoia::setParanoiaMode(int mode)
{
    mutex.lock();

    // from cdrdao 1.1.7
    paranoia_mode = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;

    switch (mode) {
    case 0:
        paranoia_mode = PARANOIA_MODE_DISABLE;
        break;
    case 1:
        paranoia_mode |= PARANOIA_MODE_OVERLAP;
        paranoia_mode &= ~PARANOIA_MODE_VERIFY;
        break;
    case 2:
        paranoia_mode &= ~(PARANOIA_MODE_SCRATCH | PARANOIA_MODE_REPAIR);
        break;
    }

    if (paranoia_never_skip)
        paranoia_mode |= PARANOIA_MODE_NEVERSKIP;

    if (paranoia)
        paranoia_modeset(paranoia, paranoia_mode);

    mutex.unlock();
}

void CDDAParanoia::setNeverSkip(bool b)
{
    paranoia_never_skip = b;
    setParanoiaMode(paranoia_mode);
}

void CDDAParanoia::setMaxRetries(int m)
{
    paranoia_max_retries = m;
}

qint16 *CDDAParanoia::paranoiaRead(void (*callback)(long, int))
{
    if (paranoia) {
        mutex.lock();
        int16_t *data = paranoia_read_limited(paranoia, callback, paranoia_max_retries);
        mutex.unlock();
        return data;
    }
    return nullptr;
}

int CDDAParanoia::paranoiaSeek(long sector, int mode)
{
    if (paranoia) {
        mutex.lock();
        long pos = paranoia_seek(paranoia, sector, mode);
        mutex.unlock();
        return pos;
    }
    return -1;
}

int CDDAParanoia::firstSectorOfTrack(int track)
{
    if (paranoia_drive) {
        mutex.lock();
        long first_sector = cdda_track_firstsector(paranoia_drive, track);
        mutex.unlock();
        return first_sector;
    }
    return -1;
}

int CDDAParanoia::lastSectorOfTrack(int track)
{
    if (paranoia_drive) {
        mutex.lock();
        long last_sector = cdda_track_lastsector(paranoia_drive, track);
        mutex.unlock();
        return last_sector;
    }
    return -1;
}

int CDDAParanoia::firstSectorOfDisc()
{
    if (paranoia_drive) {
        mutex.lock();
        long first_sector = cdda_disc_firstsector(paranoia_drive);
        mutex.unlock();
        return first_sector;
    }
    return -1;
}

int CDDAParanoia::lastSectorOfDisc()
{
    if (paranoia_drive) {
        mutex.lock();
        long last_sector = cdda_disc_lastsector(paranoia_drive);
        mutex.unlock();
        return last_sector;
    }
    return -1;
}

void CDDAParanoia::sampleOffset(const int offset)
{
    int sample_offset = offset;

    // Hack from cdda paranoia
    if (paranoia_drive) {
        mutex.lock();

        int toc_offset = 0;
        toc_offset += sample_offset / 588;
        sample_offset %= 588;
        if (sample_offset < 0) {
            sample_offset += 588;
            toc_offset--;
        }

        for (int i = 0; i < paranoia_drive->tracks + 1; ++i)
            paranoia_drive->disc_toc[i].dwStartSector += toc_offset;

        mutex.unlock();
    }
}

int CDDAParanoia::numOfTracks()
{
    if (paranoia_drive)
        return (paranoia_drive->tracks < 0) ? 0 : paranoia_drive->tracks;
    return 0;
}

int CDDAParanoia::numOfAudioTracks()
{
    if (numOfTracks() > 0) {
        int j = 0;
        for (int i = 1; i <= numOfTracks(); i++) {
            if (isAudioTrack(i))
                j++;
        }
        return j;
    }
    return 0;
}

int CDDAParanoia::length()
{
    return numOfFrames() / 75;
}

int CDDAParanoia::numOfFrames()
{
    if (numOfTracks() > 0) {
        if (paranoia_drive)
            return cdda_disc_lastsector(paranoia_drive);
    }
    return 0;
}

int CDDAParanoia::lengthOfAudioTracks()
{
    return numOfFramesOfAudioTracks() / 75;
}

int CDDAParanoia::numOfFramesOfAudioTracks()
{
    if (numOfTracks() > 0) {
        int frames = 0;
        for (int i = 1; i <= numOfTracks(); ++i) {
            if (isAudioTrack(i))
                frames += numOfFramesOfTrack(i);
        }
        return frames;
    }
    return 0;
}

int CDDAParanoia::numOfSkippedFrames(int n)
{
    if (numOfTracks() > 0) {
        if (n < 1)
            n = 1;
        if (n > numOfTracks())
            n = numOfTracks();
        int frames = 0;
        for (int i = 1; i < n; ++i) {
            if (!isAudioTrack(i))
                frames += numOfFramesOfTrack(i);
        }
        return frames;
    }
    return 0;
}

int CDDAParanoia::lengthOfTrack(int n)
{
    if (numOfTracks() > 0) {
        return numOfFramesOfTrack(n) / 75;
    }
    return 0;
}

int CDDAParanoia::numOfFramesOfTrack(int n)
{
    if (numOfTracks() > 0) {
        if (n < 1)
            n = 1;
        if (n > numOfTracks())
            n = numOfTracks();

        if (n == numOfTracks()) {
            return numOfFrames() - paranoia_drive->disc_toc[n - 1].dwStartSector;
        } else {
            return paranoia_drive->disc_toc[n].dwStartSector - paranoia_drive->disc_toc[n - 1].dwStartSector;
        }
    }
    return 0;
}

double CDDAParanoia::sizeOfTrack(int n)
{
    if (numOfTracks() > 0) {
        auto frame_size = (double)(numOfFramesOfTrack(n));
        if (isAudioTrack(n)) {
            return (frame_size * 2352.0f) / (1024.0f * 1024.0f);
        } else {
            return (frame_size * 2048.0f) / (1024.0f * 1024.0f);
        }
    }
    return 0.0f;
}

int CDDAParanoia::frameOffsetOfTrack(int n)
{
    if (numOfTracks() > 0) {
        return paranoia_drive->disc_toc[n - 1].dwStartSector;
    }
    return 0;
}

bool CDDAParanoia::isAudioTrack(int n)
{
    if (paranoia_drive)
        return IS_AUDIO(paranoia_drive, n - 1);
    return true;
}

QList<quint32> CDDAParanoia::discSignature(const qint32 pregap)
{
    QList<quint32> result;

    for (int i = 1; i <= numOfTracks() + 1; ++i)
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

    paranoia_drive = cdda_identify(_device.toLatin1().data(), 0, nullptr);
    if (paranoia_drive == nullptr) {
        mutex.unlock();
        qDebug() << "Failed to find device.";
        return false;
    }

    // cdda_cdda_verbose_set(_drive, 1, 1);

    cdda_open(paranoia_drive);
    paranoia = paranoia_init(paranoia_drive);
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
    if (paranoia_drive) {
        cdda_close(paranoia_drive);
        paranoia_drive = nullptr;
    }
    // mutex.unlock();
}
