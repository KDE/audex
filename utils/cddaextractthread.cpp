/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaextractthread.h"

#include <QDebug>

static CDDAExtractThread *aet = nullptr;

void paranoiaCallback(long sector, paranoia_cb_mode_t status)
{
    aet->createStatus(sector, status);
}

CDDAExtractThread::CDDAExtractThread(QObject *parent, CDDACDIO *cdio)
    : QThread(parent)
{
    p_cdio = cdio;
    if (!p_cdio) {
        qDebug() << "Paranoia object not found. low mem?";
        Q_EMIT error(i18n("Internal device error."), i18n("Check your device and make a bug report."));
        return;
    }
    connect(p_cdio, SIGNAL(error(const QString &, const QString &)), this, SLOT(slot_error(const QString &, const QString &)));

    overall_sectors_read = 0;
    paranoia_full_mode = true;
    paranoia_retries = 20;
    paranoia_never_skip = true;
    sample_offset = 0;
    sample_offset_done = false;
    track = 1;
    b_interrupt = false;
    b_error = false;
    read_error = false;
    scratch_detected = false;
}

CDDAExtractThread::~CDDAExtractThread()
{
}

void CDDAExtractThread::start()
{
    QThread::start();
}

void CDDAExtractThread::run()
{
    if (!p_cdio)
        return;

    if (b_interrupt)
        return;

    b_interrupt = false;
    b_error = false;

    if ((sample_offset) && (!sample_offset_done)) {
        p_cdio->sampleOffset(sample_offset);
        sample_offset_done = true;
    }

    if (track == 0) {
        first_sector = p_cdio->firstSectorOfDisc();
        last_sector = p_cdio->lastSectorOfDisc();
    } else {
        first_sector = p_cdio->firstSectorOfTrack(track);
        last_sector = p_cdio->lastSectorOfTrack(track);
    }

    if (first_sector < 0 || last_sector < 0) {
        Q_EMIT info(i18n("Extracting finished."));
        return;
    }

    qDebug() << "Sectors to read: " << QString("%1").arg(last_sector - first_sector);

    // status variable
    overlap = 0;
    read_sectors = 0;

    // track length
    sectors_all = last_sector - first_sector;
    sectors_read = 0;

    p_cdio->enableParanoiaFullMode(paranoia_full_mode);
    p_cdio->enableParanoiaNeverSkip(paranoia_never_skip);
    p_cdio->setParanoiaMaxRetries(paranoia_retries);

    p_cdio->paranoiaSeek(first_sector, SEEK_SET);
    current_sector = first_sector;

    if (track > 0) {
        QString min = QString("%1").arg((sectors_all / SECTORS_PER_SECOND) / 60, 2, 10, QChar('0'));
        QString sec = QString("%1").arg((sectors_all / SECTORS_PER_SECOND) % 60, 2, 10, QChar('0'));
        Q_EMIT info(i18n("Ripping track %1 (%2:%3)...", track, min, sec));
    } else {
        Q_EMIT info(i18n("Ripping whole CD as single track."));
    }
    extract_protocol.append(i18n("Start reading track %1 with %2 sectors", track, sectors_all));

    while (current_sector <= last_sector) {
        if (b_interrupt) {
            qDebug() << "Interrupt reading.";
            break;
        }

        // let the global paranoia callback have access to this
        // to emit signals
        aet = this;

        int16_t *buf = p_cdio->paranoiaRead(paranoiaCallback);

        if (nullptr == buf) {
            qDebug() << "Unrecoverable error in paranoia_read (sector " << current_sector << ")";
            b_error = true;
            break;

        } else {
            current_sector++;
            QByteArray a((char *)buf, CD_FRAMESIZE_RAW);
            Q_EMIT output(a);
            a.clear();

            sectors_read++;
            overall_sectors_read++;
            float fraction = 0.0f;
            if (sectors_all > 0)
                fraction = (float)sectors_read / (float)sectors_all;
            Q_EMIT progress((int)(100.0f * fraction), current_sector, overall_sectors_read);
        }
    }

    if (b_interrupt)
        Q_EMIT error(i18n("User canceled extracting."));

    if (b_error)
        Q_EMIT error(i18n("An error occurred while ripping track %1.", track));

    if ((!b_interrupt) && (!b_error)) {
        if (track > 0) {
            Q_EMIT info(i18n("Ripping OK (Track %1).", track));
        } else {
            Q_EMIT info(i18n("Ripping OK."));
        }
    }

    qDebug() << "Reading finished.";
    extract_protocol.append(i18n("Reading finished"));
}

void CDDAExtractThread::cancel()
{
    b_interrupt = true;
}

bool CDDAExtractThread::isProcessing()
{
    return !(b_interrupt || !isRunning());
}

const QStringList &CDDAExtractThread::protocol()
{
    return extract_protocol;
}

void CDDAExtractThread::slot_error(const QString &message, const QString &details)
{
    Q_EMIT error(message, details);
}

void CDDAExtractThread::createStatus(long sector, paranoia_cb_mode_t status)
{
    sector /= CD_FRAMESIZE_RAW / 2;
    QString tp_min = QString("%1").arg((current_sector / SECTORS_PER_SECOND) / 60, 2, 10, QChar('0'));
    QString tp_sec = QString("%1").arg((current_sector / SECTORS_PER_SECOND) % 60, 2, 10, QChar('0'));

    switch (status) {
    case PARANOIA_CB_READ:
        // no problem
        read_sectors++;
        read_error = false;
        scratch_detected = false;
        break;
    case PARANOIA_CB_VERIFY:
        // qDebug() << "Verifying jitter";
        break;
    case PARANOIA_CB_FIXUP_EDGE:
        qDebug() << "Fixed edge jitter";
        extract_protocol.append(
            i18n("Fixed edge jitter (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        break;
    case PARANOIA_CB_FIXUP_ATOM:
        qDebug() << "Fixed atom jitter";
        extract_protocol.append(
            i18n("Fixed atom jitter (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        break;
    case PARANOIA_CB_SCRATCH:
        // scratch detected
        qDebug() << "Scratch detected";
        if (!scratch_detected) {
            scratch_detected = true;
            Q_EMIT warning(i18n("Scratch detected (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        }
        extract_protocol.append(
            i18n("SCRATCH DETECTED (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        break;
    case PARANOIA_CB_REPAIR:
        qDebug() << "Repair";
        extract_protocol.append(i18n("Repair (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        break;
    case PARANOIA_CB_SKIP:
        // skipped sector
        qDebug() << "Skip";
        Q_EMIT warning(i18n("Skip sectors (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        extract_protocol.append(i18n("SKIP (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        break;
    case PARANOIA_CB_DRIFT:
        qDebug() << "Drift";
        extract_protocol.append(i18n("Drift (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        break;
    case PARANOIA_CB_BACKOFF:
        qDebug() << "Backoff";
        extract_protocol.append(i18n("Backoff (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        break;
    case PARANOIA_CB_OVERLAP:
        // sector does not seem to contain the current
        // sector but the amount of overlapped data
        // qDebug() << "overlap.";
        overlap = sector;
        break;
    case PARANOIA_CB_FIXUP_DROPPED:
        qDebug() << "Fixup dropped";
        extract_protocol.append(i18n("Fixup dropped (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        break;
    case PARANOIA_CB_FIXUP_DUPED:
        qDebug() << "Fixup duped";
        extract_protocol.append(i18n("Fixup duped (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        break;
    case PARANOIA_CB_READERR:
        qDebug() << "Read error";
        if (!read_error) {
            read_error = true;
            Q_EMIT warning(i18n("Read error detected (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        }
        extract_protocol.append(i18n("READ ERROR (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
        break;
    case PARANOIA_CB_CACHEERR:
        qDebug() << "Bad cache management";
        break;
    case PARANOIA_CB_WROTE:
        break;
    case PARANOIA_CB_FINISHED:
        break;
    }
}
