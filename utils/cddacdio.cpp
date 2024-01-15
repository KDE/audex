/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddacdio.h"

#include <QDebug>
#include <cdio/track.h>

/* some of this code in here is based on k3b 0.8.x sourcecode */

CDDACDIO::CDDACDIO(QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(parent);
    paranoia = nullptr;
    drive = nullptr;
    enableParanoiaNeverSkip();
    setParanoiaMaxRetries(20);
    enableParanoiaFullMode();
}

CDDACDIO::~CDDACDIO()
{
    _paranoia_free();
}

bool CDDACDIO::setDevice(const QString &device)
{
    if (device.isEmpty())
        this->device = "/dev/cdrom";
    if (!device.isEmpty())
        this->device = device;
    if (!_paranoia_init()) {
        qDebug() << "Internal device error.";
        Q_EMIT error(i18n("Internal device error."), i18n("Check your device. Is it really \"%1\"? If so also check your permissions on \"%1\".", device));
        return false;
    }
    detectHardware();
    return true;
}

QString CDDACDIO::getDevice() const
{
    return device;
}

bool CDDACDIO::detectHardware()
{
    char buf[36] = {
        0,
    };
    mmc_cdb_t cdb = {{
        0,
    }};

    CdIo_t *p_cdio = cdio_open(device.toLatin1().constData(), DRIVER_UNKNOWN);

    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_INQUIRY);
    cdb.field[4] = sizeof(buf);

    int i_status = mmc_run_cmd(p_cdio, 100000, &cdb, SCSI_MMC_DATA_READ, sizeof(buf), &buf);
    if (i_status == 0) {
        char psz_vendor[CDIO_MMC_HW_VENDOR_LEN + 1];
        char psz_model[CDIO_MMC_HW_MODEL_LEN + 1];
        char psz_rev[CDIO_MMC_HW_REVISION_LEN + 1];

        vendor = QByteArray(buf + 8, sizeof(psz_vendor) - 1).trimmed();
        model = QByteArray(buf + 8 + CDIO_MMC_HW_VENDOR_LEN, sizeof(psz_model) - 1).trimmed();
        revision = QByteArray(buf + 8 + CDIO_MMC_HW_VENDOR_LEN + CDIO_MMC_HW_MODEL_LEN, sizeof(psz_rev) - 1).trimmed();

        qDebug() << "Vendor:" << vendor << ", Model:" << model << ", Revision:" << revision;

        return true;

    } else {
        Q_EMIT error(i18n("Could not get device hardware information (vendor, model and firmware revision)."),
                     i18n("Check your device. Is it really \"%1\"? If so also check your permissions on \"%1\".", device));
    }

    return false;
}

const QString CDDACDIO::getVendor() const
{
    return vendor;
}

const QString CDDACDIO::getModel() const
{
    return model;
}

const QString CDDACDIO::getRevision() const
{
    return revision;
}

void CDDACDIO::enableParanoiaFullMode(const bool enabled)
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

void CDDACDIO::enableParanoiaNeverSkip(const bool never_skip)
{
    paranoia_never_skip = never_skip;
    if (paranoia_never_skip)
        paranoia_mode |= PARANOIA_MODE_NEVERSKIP;
    else
        paranoia_mode &= ~PARANOIA_MODE_NEVERSKIP;

    if (paranoia)
        paranoia_modeset(paranoia, paranoia_mode);
}

void CDDACDIO::setParanoiaMaxRetries(int max_retries)
{
    paranoia_max_retries = max_retries;
}

qint16 *CDDACDIO::paranoiaRead(void (*callback)(long, paranoia_cb_mode_t))
{
    if (paranoia) {
        mutex.lock();
        int16_t *data = cdio_paranoia_read_limited(paranoia, callback, paranoia_max_retries);
        mutex.unlock();
        return data;
    }
    return nullptr;
}

int CDDACDIO::paranoiaSeek(long sector, qint32 mode)
{
    if (paranoia) {
        mutex.lock();
        long pos = cdio_paranoia_seek(paranoia, sector, mode);
        mutex.unlock();
        return pos;
    }
    return -1;
}

int CDDACDIO::firstSectorOfTrack(track_t track)
{
    if (drive) {
        mutex.lock();
        long first_sector = cdio_cddap_track_firstsector(drive, track);
        mutex.unlock();
        return first_sector;
    }
    return -1;
}

int CDDACDIO::lastSectorOfTrack(track_t track)
{
    if (drive) {
        mutex.lock();
        long last_sector = cdio_cddap_track_lastsector(drive, track);
        mutex.unlock();
        return last_sector;
    }
    return -1;
}

int CDDACDIO::firstSectorOfDisc()
{
    if (drive) {
        mutex.lock();
        long first_sector = cdio_cddap_disc_firstsector(drive);
        mutex.unlock();
        return first_sector;
    }
    return -1;
}

int CDDACDIO::lastSectorOfDisc()
{
    if (drive) {
        mutex.lock();
        long last_sector = cdio_cddap_disc_lastsector(drive);
        mutex.unlock();
        return last_sector;
    }
    return -1;
}

void CDDACDIO::sampleOffset(const int offset)
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

track_t CDDACDIO::numOfTracks() const
{
    if (drive)
        return cdio_cddap_tracks(drive);
    return 0;
}

track_t CDDACDIO::numOfAudioTracks() const
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

int CDDACDIO::length() const
{
    return numOfFrames() / FRAMES_PER_SECOND;
}

int CDDACDIO::numOfFrames() const
{
    if (numOfTracks() > 0) {
        if (drive)
            return cdio_cddap_disc_lastsector(drive);
    }
    return 0;
}

int CDDACDIO::lengthOfAudioTracks() const
{
    return numOfFramesOfAudioTracks() / FRAMES_PER_SECOND;
}

int CDDACDIO::numOfFramesOfAudioTracks() const
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

int CDDACDIO::numOfSkippedFrames(int n) const
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

int CDDACDIO::lengthOfTrack(track_t track) const
{
    if (numOfTracks() > 0) {
        return numOfFramesOfTrack(track) / FRAMES_PER_SECOND;
    }
    return 0;
}

int CDDACDIO::numOfFramesOfTrack(track_t track) const
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

double CDDACDIO::sizeOfTrack(track_t track) const
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

int CDDACDIO::frameOffsetOfTrack(track_t track) const
{
    if (numOfTracks() > 0 && drive)
        return cdio_cddap_track_firstsector(drive, track);
    return 0;
}

bool CDDACDIO::isAudioTrack(track_t track) const
{
    if (drive)
        return cdio_cddap_track_audiop(drive, track) == 1;
    return true;
}

QList<quint32> CDDACDIO::discSignature(const qint32 pregap)
{
    QList<quint32> result;

    for (track_t i = 1; i <= numOfTracks() + 1; ++i)
        result.append(frameOffsetOfTrack(i) + pregap);

    return result;
}

void CDDACDIO::reset()
{
    _paranoia_init();
}

bool CDDACDIO::_paranoia_init()
{
    mutex.lock();

    _paranoia_free();

    drive = cdda_identify(device.toLatin1().data(), 0, nullptr);
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

void CDDACDIO::_paranoia_free()
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
