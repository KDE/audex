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
    cdio = nullptr;
    enableParanoiaNeverSkip();
    setParanoiaMaxRetries(20);
    enableParanoiaFullMode();
}

CDDACDIO::~CDDACDIO()
{
    p_paranoia_free();
}

bool CDDACDIO::setDevice(const QString &device)
{
    if (device.isEmpty())
        this->device = "/dev/cdrom";
    if (!device.isEmpty())
        this->device = device;

    if (!p_paranoia_init()) {
        qDebug() << "Internal device error.";
        qDebug() << "Unable to create paranoia object.";
        Q_EMIT error(i18n("Internal device error."), i18n("Check your device. Is it really \"%1\"? If so also check your permissions on \"%1\".", device));
        return false;
    }

    cdio = cdio_open(device.toLatin1().constData(), DRIVER_UNKNOWN);
    if (!cdio) {
        qDebug() << "Unable to create cdio object.";
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

    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_INQUIRY);
    cdb.field[4] = sizeof(buf);

    int i_status = mmc_run_cmd(cdio, 100000, &cdb, SCSI_MMC_DATA_READ, sizeof(buf), &buf);
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

int CDDACDIO::paranoiaSeek(const int sector, qint32 mode)
{
    if (paranoia) {
        mutex.lock();
        long pos = cdio_paranoia_seek(paranoia, sector, mode);
        mutex.unlock();
        return pos;
    }
    return -1;
}

int CDDACDIO::firstSectorOfTrack(const int track)
{
    if (cdio) {
        mutex.lock();
        int first_sector = cdio_get_track_lsn(cdio, track);
        mutex.unlock();
        return first_sector;
    }
    return -1;
}

int CDDACDIO::lastSectorOfTrack(const int track)
{
    if (cdio) {
        mutex.lock();
        long last_sector = cdio_get_track_last_lsn(cdio, track);
        mutex.unlock();
        return last_sector;
    }
    return -1;
}

int CDDACDIO::firstSectorOfDisc()
{
    return 0;
}

int CDDACDIO::lastSectorOfDisc()
{
    if (drive) {
        mutex.lock();
        int last_sector = cdio_get_disc_last_lsn(cdio) - 1;
        mutex.unlock();
        return last_sector;
    }
    return -1;
}

int CDDACDIO::firstTrackNum()
{
    if (cdio) {
        mutex.lock();
        int t = cdio_get_first_track_num(cdio);
        mutex.unlock();
        return t;
    }
    return -1;
}

int CDDACDIO::lastTrackNum()
{
    if (cdio) {
        mutex.lock();
        int t = cdio_get_last_track_num(cdio);
        mutex.unlock();
        return t;
    }
    return -1;
}

int CDDACDIO::numOfTracks()
{
    if (cdio) {
        mutex.lock();
        int num = cdio_get_num_tracks(cdio);
        mutex.unlock();
        return num;
    }
    return -1;
}

int CDDACDIO::numOfAudioTracks()
{
    if (cdio && numOfTracks() > 0) {
        int num = 0;
        mutex.lock();
        for (int t = firstTrackNum(); t <= lastTrackNum(); ++t)
            if (isAudioTrack(t))
                ++num;
        mutex.unlock();
        return num;
    }
    return 0;
}

int CDDACDIO::length()
{
    return numOfFrames() / FRAMES_PER_SECOND;
}

int CDDACDIO::numOfFrames()
{
    if (numOfTracks() > 0) {
        if (drive)
            return cdio_cddap_disc_lastsector(drive);
    }
    return 0;
}

int CDDACDIO::lengthOfAudioTracks()
{
    return lastSectorOfDisc() / FRAMES_PER_SECOND;
}

int CDDACDIO::numOfFramesOfAudioTracks()
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

int CDDACDIO::numOfSkippedFrames(const int n)
{
    if (cdio && numOfTracks() > 0) {
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

int CDDACDIO::lengthOfTrack(const int track)
{
    if (cdio && numOfTracks() > 0) {
        return numOfFramesOfTrack(track) / FRAMES_PER_SECOND;
    }
    return 0;
}

int CDDACDIO::numOfFramesOfTrack(const int track)
{
    if (cdio && numOfTracks() > 0) {
        int t = track;
        if (t < 1)
            t = 1;
        if (t > numOfTracks())
            t = numOfTracks();
        return lastSectorOfTrack(t) - firstSectorOfTrack(t);
    }
    return 0;
}

const QString CDDACDIO::getMCN()
{
    QString result;

    if (!p_cache_mcn.isEmpty())
        return p_cache_mcn;

    if (cdio) {
        mutex.lock();
        char *mcn = cdio_get_mcn(cdio);
        result = QString::fromLatin1(mcn, -1);
        p_cache_mcn = result;
        cdio_free(mcn);
        mutex.unlock();
    }

    return result;
}

const QString CDDACDIO::getISRC(const int track)
{
    QString result;

    if (p_cache_isrc.contains(track))
        return p_cache_isrc.value(track);

    if (cdio) {
        mutex.lock();
        char *isrc = cdio_get_track_isrc(cdio, track);
        result = QString::fromLatin1(isrc, -1);
        p_cache_isrc.insert(track, result);
        cdio_free(isrc);
        mutex.unlock();
    }

    return result;
}

const QString CDDACDIO::msfOfTrack(const int track)
{
    return LSN2MSF(firstSectorOfTrack(track));
}

qreal CDDACDIO::sizeOfTrack(const int track)
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

int CDDACDIO::frameOffsetOfTrack(const int track)
{
    return firstSectorOfTrack(track);
}

bool CDDACDIO::isAudioTrack(const int track)
{
    if (cdio) {
        mutex.lock();
        bool is_audio = cdio_get_track_format(cdio, track) == TRACK_FORMAT_AUDIO;
        mutex.unlock();
        return is_audio;
    }
    return true;
}

bool CDDACDIO::isLastTrack(const int track)
{
    return track == lastTrackNum();
}

QList<quint32> CDDACDIO::discSignature()
{
    QList<quint32> result;

    if (cdio)
        for (track_t t = 1; t <= numOfTracks() + 1; ++t)
            result.append(cdio_get_track_lba(cdio, t));

    return result;
}

void CDDACDIO::reset()
{
    p_paranoia_init();
    p_cache_mcn.clear();
    p_cache_isrc.clear();
}

bool CDDACDIO::p_paranoia_init()
{
    mutex.lock();

    p_paranoia_free();

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
        p_paranoia_free();
        mutex.unlock();
        qDebug() << "Failed to init device.";
        return false;
    }

    mutex.unlock();
    return true;
}

void CDDACDIO::p_paranoia_free()
{
    if (paranoia) {
        paranoia_free(paranoia);
        paranoia = nullptr;
    }
    if (drive) {
        cdda_close(drive);
        drive = nullptr;
    }
    if (cdio) {
        cdio_free(cdio);
        cdio = nullptr;
    }
}
