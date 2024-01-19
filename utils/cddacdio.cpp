/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddacdio.h"
#include "cddaextractthread.h"

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

    enableParanoiaNeverSkip(false);
    setParanoiaMaxRetries(20);
    enableParanoiaFullMode();
}

CDDACDIO::~CDDACDIO()
{
    p_free();
}

bool CDDACDIO::init(const QString &deviceFile)
{
    reset();
    p_free();

    device_file = deviceFile;

    drive = cdda_identify(device_file.toLocal8Bit().constData(), 0, nullptr);
    if (!drive) {
        qDebug() << "Failed to find device.";
        Q_EMIT error(i18n("Device error (1)."), i18n("Check your device. Is it really \"%1\"? Also check your permissions on \"%1\".", device_file));
        return false;
    }

    cdda_open(drive);
    paranoia = paranoia_init(drive);
    if (paranoia == nullptr) {
        Q_EMIT error(i18n("Device error (2)."), i18n("Check your device. Is it really \"%1\"? Also check your permissions on \"%1\".", device_file));
        p_free();
        return false;
    }

    cdio = cdio_open(device_file.toLocal8Bit().constData(), DRIVER_UNKNOWN);
    if (!cdio) {
        qDebug() << "Unable to create cdio object.";
        Q_EMIT error(i18n("Device error (3)."), i18n("Check your device. Is it really \"%1\"? Also check your permissions on \"%1\".", device_file));
        p_free();
        return false;
    }

    cdio_drive_read_cap_t read_caps;
    cdio_drive_write_cap_t write_caps;
    cdio_drive_misc_cap_t misc_caps;

    cdio_get_drive_cap(cdio, &read_caps, &write_caps, &misc_caps);

    capabilities.clear();
    if (misc_caps & CDIO_DRIVE_CAP_MISC_CLOSE_TRAY)
        capabilities.insert(DriveCapability::CLOSE_TRAY);
    if (misc_caps & CDIO_DRIVE_CAP_MISC_EJECT)
        capabilities.insert(DriveCapability::EJECT);
    if (misc_caps & CDIO_DRIVE_CAP_MISC_LOCK)
        capabilities.insert(DriveCapability::LOCK);
    if (misc_caps & CDIO_DRIVE_CAP_MISC_SELECT_SPEED)
        capabilities.insert(DriveCapability::SELECT_SPEED);
    if (misc_caps & CDIO_DRIVE_CAP_MISC_SELECT_DISC)
        capabilities.insert(DriveCapability::SELECT_DISC);
    if (misc_caps & CDIO_DRIVE_CAP_MISC_MULTI_SESSION)
        capabilities.insert(DriveCapability::READ_MULTI_SESSION);
    if (misc_caps & CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED)
        capabilities.insert(DriveCapability::MEDIA_CHANGED);

    if (read_caps & CDIO_DRIVE_CAP_READ_CD_DA)
        capabilities.insert(DriveCapability::READ_CDDA);
    if (read_caps & CDIO_DRIVE_CAP_READ_C2_ERRS)
        capabilities.insert(DriveCapability::C2_ERRS);
    if (read_caps & CDIO_DRIVE_CAP_READ_MODE2_FORM1)
        capabilities.insert(DriveCapability::READ_MODE2_FORM1);
    if (read_caps & CDIO_DRIVE_CAP_READ_MODE2_FORM2)
        capabilities.insert(DriveCapability::READ_MODE2_FORM2);
    if (read_caps & CDIO_DRIVE_CAP_READ_MCN)
        capabilities.insert(DriveCapability::READ_MCN);
    if (read_caps & CDIO_DRIVE_CAP_READ_ISRC)
        capabilities.insert(DriveCapability::READ_ISRC);

    p_detect_hardware();
    return true;
}

void CDDACDIO::reset()
{
    mcn.clear();
    track_isrcs.clear();
}

const QString CDDACDIO::getDeviceFile() const
{
    return device_file;
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

const DriveCapabilities CDDACDIO::getDriveCapabilities() const
{
    return capabilities;
}

void CDDACDIO::enableParanoiaFullMode(const bool enabled)
{
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
    if (paranoia)
        return cdio_paranoia_read_limited(paranoia, callback, paranoia_max_retries);
    return nullptr;
}

int CDDACDIO::paranoiaSeek(const int sector, qint32 mode)
{
    if (paranoia)
        return (int)cdio_paranoia_seek(paranoia, sector, mode);
    return -1;
}

bool CDDACDIO::paranoiaError(QString &errorMsg)
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

bool CDDACDIO::mediaChanged()
{
    if (cdio)
        return cdio_get_media_changed(cdio) == 1;
    return false;
}

int CDDACDIO::firstSectorOfTrack(const int track)
{
    if (cdio)
        return (int)cdio_get_track_lsn(cdio, track);
    return -1;
}

int CDDACDIO::lastSectorOfTrack(const int track)
{
    if (cdio)
        return (int)cdio_get_track_last_lsn(cdio, track);
    return -1;
}

int CDDACDIO::firstSectorOfDisc()
{
    return 0;
}

int CDDACDIO::lastSectorOfDisc()
{
    if (drive)
        return (int)cdio_get_disc_last_lsn(cdio) - 1; // last lsn ist start sector of leadout, so -1
    return -1;
}

int CDDACDIO::firstTrackNum()
{
    if (cdio)
        return (int)cdio_get_first_track_num(cdio);
    return -1;
}

int CDDACDIO::lastTrackNum()
{
    if (cdio)
        return (int)cdio_get_last_track_num(cdio);
    return -1;
}

int CDDACDIO::numOfTracks()
{
    if (cdio)
        return (int)cdio_get_num_tracks(cdio);
    return -1;
}

int CDDACDIO::numOfAudioTracks()
{
    if (cdio && numOfTracks() > 0) {
        int num = 0;
        for (int t = firstTrackNum(); t <= lastTrackNum(); ++t)
            if (isAudioTrack(t))
                ++num;
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
    if (!capabilities.contains(DriveCapability::READ_MCN))
        return QString();

    if (cdio && mcn.isEmpty()) {
        char *data = cdio_get_mcn(cdio);
        QString string = QString::fromLocal8Bit(data, -1);
        if (!string.isEmpty() && string != "0")
            mcn = string;
        cdio_free(data);
    }

    return mcn;
}

const QString CDDACDIO::getISRC(const int track)
{
    if (!capabilities.contains(DriveCapability::READ_ISRC))
        return QString();

    if (cdio && !track_isrcs.contains(track)) {
        char *data = cdio_get_track_isrc(cdio, track);
        QString string = QString::fromLocal8Bit(data, -1);
        if (!string.isEmpty() && string != "0")
            track_isrcs.insert(track, string);
        cdio_free(data);
    }

    return track_isrcs.value(track, QString());
}

bool CDDACDIO::isPreemphasis(const int track)
{
    if (drive && isAudioTrack(track))
        return cdio_cddap_track_preemp(drive, track) == 1;
    return false;
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
    if (cdio)
        return cdio_get_track_format(cdio, track) == TRACK_FORMAT_AUDIO;
    return true;
}

bool CDDACDIO::isDataTrack(const int track)
{
    if (cdio)
        return cdio_get_track_format(cdio, track) == TRACK_FORMAT_DATA;
    return true;
}

bool CDDACDIO::isLastTrack(const int track)
{
    return track == lastTrackNum();
}

bool CDDACDIO::isAudioDisc()
{
    if (cdio) {
        for (track_t t = 1; t <= numOfTracks(); ++t)
            if (cdio_get_track_format(cdio, t) != TRACK_FORMAT_AUDIO)
                return false;
    } else {
        return false;
    }
    return true;
}

QList<quint32> CDDACDIO::discSignature()
{
    QList<quint32> result;

    if (cdio)
        for (track_t t = 1; t <= numOfTracks() + 1; ++t)
            result.append(cdio_get_track_lba(cdio, t));

    return result;
}

const QStringList CDDACDIO::prettyTOC()
{
    QStringList result;

    result.append(i18n("Track --, Start sector: 000000, Size in sectors: 000150, Type: Lead-in"));

    if (cdio)
        for (track_t t = 1; t <= numOfTracks() + 1; ++t) {
            QString track;
            track = QString("%1").arg(t, 2, 10, QChar('0'));
            if (t == numOfTracks() + 1)
                track = "--";

            QString start = QString("%1").arg(cdio_get_track_lba(cdio, t), 6, 10, QChar('0'));

            QString size;
            int size_i = cdio_get_track_last_lsn(cdio, t) - cdio_get_track_lba(cdio, t) + 150;
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
            switch (cdio_get_track_format(cdio, t)) {
            case TRACK_FORMAT_AUDIO:
                type = i18n("Audio");
                break;
            case TRACK_FORMAT_CDI:
                type = i18n("Non-Audio: CD-i");
                break;
            case TRACK_FORMAT_XA:
                type = i18n("Non-Audio: Data mode 2");
                if (cdio_get_track_green(cdio, t))
                    type += i18n(", form 1");
                else
                    type += i18n(", form 2");
                break;
            case TRACK_FORMAT_DATA:
                type = i18n("Non-Audio: Data mode 1");
                break;
            case TRACK_FORMAT_PSX:
                type = i18n("Non-Audio: Playstation 1");
                break;
            case TRACK_FORMAT_ERROR:
                type = i18n("Non-Audio: Unknown format");
                break;
            }
            if (t == numOfTracks() + 1)
                type = i18n("Lead-out");

            result.append(i18n("Track %1, Start sector: %2, Size in sectors: %3, Type: %4", track, start, size, type));
        }

    return result;
}

void CDDACDIO::p_detect_hardware()
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

    } else {
        Q_EMIT error(i18n("Could not get device hardware information (vendor, model and firmware revision)."),
                     i18n("Check your device. Is it really \"%1\"? Also check your permissions on \"%1\".", device_file));
    }
}

void CDDACDIO::p_free()
{
    if (paranoia) {
        paranoia_free(paranoia);
        paranoia = nullptr;
    }
    if (drive) {
        cdio_cddap_close_no_free_cdio(drive);
        drive = nullptr;
    }
    if (cdio) {
        cdio_destroy(cdio);
        cdio = nullptr;
    }
}
