/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaextractthread.h"

#include <QDebug>
#include <cdio/sector.h>

static CDDAExtractThread *aet = nullptr;

void paranoia_callback(long sector, paranoia_cb_mode_t status)
{
    aet->create_status(sector, status);
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
    paranoia_never_skip = false;
    skip_reading_errors = false;
    sample_offset = 0;
    track = 1;
    b_first_run = true;
    b_interrupt = false;
    b_error = false;
    status_previous_sector = -1;

    silence.fill(0, CD_FRAMESIZE_RAW);
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

    paranoia_status_count.clear();
    paranoia_status_table.clear();

    if (b_first_run) {
        p_log.append(i18n("Drive Vendor: %1, Drive Model: %2, Drive Revision: %3", p_cdio->getVendor(), p_cdio->getModel(), p_cdio->getRevision()));
        p_log.append(i18n("TOC:"));
        p_log.append(p_cdio->prettyTOC());
        b_first_run = false;
    }

    if (track == 0) {
        first_sector = p_cdio->firstSectorOfDisc();
        last_sector = p_cdio->lastSectorOfDisc();
    } else {
        first_sector = p_cdio->firstSectorOfTrack(track) + sector_offset;
        last_sector = p_cdio->lastSectorOfTrack(track) + sector_offset;
    }

    if (first_sector < 0 || last_sector < 0) {
        Q_EMIT info(i18n("Extracting finished."));
        return;
    }

    qDebug() << "Track:" << track;
    qDebug() << "Sample offset:" << sample_offset;
    qDebug() << "Sector offset:" << sector_offset;
    qDebug() << "Sample offset fraction:" << sample_offset_fraction;
    qDebug() << "First sector:" << first_sector;
    qDebug() << "Last sector:" << last_sector;

    // track length
    sectors_all = last_sector - first_sector;
    sectors_all += sector_offset;
    sectors_read = 0;

    p_cdio->enableParanoiaFullMode(paranoia_full_mode);
    p_cdio->enableParanoiaNeverSkip(paranoia_never_skip);
    p_cdio->setParanoiaMaxRetries(paranoia_retries);

    QString paranoiaErrorMsg;

    p_cdio->paranoiaSeek(first_sector, SEEK_SET);
    if (p_cdio->paranoiaError(paranoiaErrorMsg)) {
        p_log.append(i18n("Error occured while seeking at sector %1: %2", first_sector, paranoiaErrorMsg));
        if (track > 0)
            Q_EMIT error(i18n("An error occured while ripping track %1. See log.", track));
        else
            Q_EMIT error(i18n("An error occured while ripping. See log."));
        return;
    }

    current_sector = first_sector;

    if (sample_offset > 0)
        p_log.append(i18n("Correction sample offset: %1", sample_offset));

    QString min = QString("%1").arg((sectors_all / SECTORS_PER_SECOND) / 60, 2, 10, QChar('0'));
    QString sec = QString("%1").arg((sectors_all / SECTORS_PER_SECOND) % 60, 2, 10, QChar('0'));

    if (track > 0) {
        Q_EMIT info(i18n("Ripping track %1 (%2:%3)...", track, min, sec));
        p_log.append(i18n("Start reading track %1 with %2 sectors", track, sectors_all));
    } else {
        Q_EMIT info(i18n("Ripping whole disc (%1:%2)...", min, sec));
        p_log.append(i18n("Start reading whole disc with %1 sectors", sectors_all));
        p_log.append(i18n("Track %1 with start sector %2", p_cdio->firstTrackNum(), p_cdio->firstSectorOfTrack(p_cdio->firstTrackNum()) + sector_offset));
    }

    p_log.append(i18n("First sector: %1, Last sector: %2", first_sector, last_sector));

    p_cdio->mediaChanged();

    bool overread = false;
    while (current_sector <= last_sector || overread) {
        if (b_interrupt) {
            qDebug() << "Interrupt ripping";
            break;
        }

        // let the global paranoia callback have access to this to emit signals
        aet = this;

        if (p_cdio->mediaChanged()) {
            b_interrupt = true;
            continue;
        }

        int16_t *buf = p_cdio->paranoiaRead(paranoia_callback);
        if (p_cdio->paranoiaError(paranoiaErrorMsg)) {
            p_log.append(i18n("Error occured while reading sector %1 (track time pos %2): %3",
                              current_sector,
                              CDDACDIO::LSN2MSF(current_sector - first_sector, QChar('-')),
                              paranoiaErrorMsg));
            if (!skip_reading_errors) {
                if (track > 0)
                    Q_EMIT error(i18n("An error occured while ripping track %1 at position %2. See log.",
                                      track,
                                      CDDACDIO::LSN2MSF(current_sector - first_sector, QChar('-'))));
                else
                    Q_EMIT error(i18n("An error occured while ripping at position %1. See log.", CDDACDIO::LSN2MSF(current_sector - first_sector, QChar('-'))));
                b_error = true;
                break;
            } else {
                if (track > 0)
                    Q_EMIT warning(i18n("An error occured while ripping track %1 at position %2. See log.",
                                        track,
                                        CDDACDIO::LSN2MSF(current_sector - first_sector, QChar('-'))));
                else
                    Q_EMIT warning(
                        i18n("An error occured while ripping at position %1. See log.", CDDACDIO::LSN2MSF(current_sector - first_sector, QChar('-'))));
            }
        }
        if (!buf) {
            if (paranoiaErrorMsg.isEmpty()) {
                p_log.append(i18n("Error reading sector %1 (track time pos %2)", current_sector, CDDACDIO::LSN2MSF(current_sector - first_sector, QChar('-'))));
                if (paranoia_never_skip)
                    Q_EMIT error(i18n("An error occured while ripping at position %1. See log.", CDDACDIO::LSN2MSF(current_sector - first_sector, QChar('-'))));
                else
                    Q_EMIT warning(
                        i18n("An error occured while ripping at position %1. See log.", CDDACDIO::LSN2MSF(current_sector - first_sector, QChar('-'))));
            }
            if (!skip_reading_errors) {
                b_error = true;
                break;
            }
            p_log.append(i18n("Error reading sector %1 (%2): **Filling whole sector with silence**",
                              current_sector,
                              CDDACDIO::LSN2MSF(current_sector - first_sector, QChar('-'))));
            buf = reinterpret_cast<int16_t *>(silence.data());
        }

        if (sample_offset_fraction > 0 && current_sector == first_sector && track > 0) {
            Q_EMIT output(QByteArray((const char *)buf + (sample_offset_fraction * 4), CD_FRAMESIZE_RAW - (sample_offset_fraction * 4)));
        } else if (sample_offset_fraction < 0 && current_sector == first_sector && track > 0) {
            Q_EMIT output(QByteArray((const char *)buf + (CD_FRAMESIZE_RAW - (-sample_offset_fraction * 4)), (-sample_offset_fraction * 4)));
        } else if (sample_offset_fraction < 0 && current_sector == last_sector && track > 0) {
            Q_EMIT output(QByteArray((const char *)buf, CD_FRAMESIZE_RAW - (-sample_offset_fraction * 4)));
        } else if (overread) {
            Q_EMIT output(QByteArray((const char *)buf, sample_offset_fraction * 4));
            overread = false;
        } else {
            Q_EMIT output(QByteArray((const char *)buf, CD_FRAMESIZE_RAW));
        }

        // if we have a positive sample offset we need to overread at the end
        if (sample_offset > 0 && current_sector == last_sector && track > 0) {
            if (p_cdio->mediaChanged()) {
                b_interrupt = true;
                break;
            }
            if (p_cdio->isLastTrack(track)) { // if we read into the leadout at the end of the disc then..
                p_cdio->paranoiaSeek(current_sector, SEEK_SET); // flush the buffer (paranoia internal)
                if (p_cdio->paranoiaError(paranoiaErrorMsg)) {
                    p_log.append(i18n("Error occured while seeking at sector %1: %2", current_sector, paranoiaErrorMsg));
                    if (track > 0)
                        Q_EMIT error(i18n("An error occured while ripping track %1. See log.", track));
                    else
                        Q_EMIT error(i18n("An error occured while ripping. See log."));
                    b_error = true;
                    break;
                }
            }
            overread = true;
        }

        ++current_sector;

        ++sectors_read;
        ++overall_sectors_read;
        float fraction = 0.0f;
        if (sectors_all > 0)
            fraction = (float)sectors_read / (float)sectors_all;
        Q_EMIT progress((int)(100.0f * fraction), current_sector, overall_sectors_read);
    }

    if (b_error) {
        Q_EMIT error(i18n("Ripping was canceled due to an error."));
        p_log.append(i18n("Ripping was canceled due to an error."));
    } else if (b_interrupt) {
        Q_EMIT error(i18n("User canceled extracting."));
        p_log.append(i18n("Extraction interrupted"));
    } else {
        if (track > 0) {
            Q_EMIT info(i18n("Ripping OK (Track %1).", track));
        } else {
            Q_EMIT info(i18n("Ripping OK."));
        }
        p_log.append(i18n("Ripping finished"));
    }
}

void CDDAExtractThread::cancel()
{
    b_interrupt = true;
}

bool CDDAExtractThread::isProcessing()
{
    return !(b_interrupt || !isRunning());
}

const QStringList &CDDAExtractThread::log()
{
    return p_log;
}

void CDDAExtractThread::reset()
{
    overall_sectors_read = 0;
    paranoia_full_mode = true;
    paranoia_retries = 20;
    paranoia_never_skip = true;
    sample_offset = 0;
    track = 1;
    b_first_run = true;
    b_interrupt = false;
    b_error = false;
    status_previous_sector = -1;
}

void CDDAExtractThread::slot_error(const QString &message, const QString &details)
{
    Q_EMIT error(message, details);
}

void CDDAExtractThread::create_status(long sector, paranoia_cb_mode_t status)
{
    if (status == PARANOIA_CB_READ || status == PARANOIA_CB_FINISHED)
        return;

    paranoia_status_count[status]++;
    paranoia_status_table.insert(sector, status);
}

/* Paranoia status explanations:
 *
 * PARANOIA_CB_READ             : No error.
 * PARANOIA_CB_VERIFY           : No error. Verifying jitter
 * PARANOIA_CB_FIXUP_EDGE       : Recoverable minor error. Fixed edge jitter.
 * PARANOIA_CB_FIXUP_ATOM       : Recoverable minor error. Fixed atom jitter.
 * PARANOIA_CB_SCRATCH          : Unsupported with current paranoia implementation. Should not occur.
 * PARANOIA_CB_REPAIR           : Unsupported with current paranoia implementation. Should not occur.
 * PARANOIA_CB_SKIP             : Error. Skipped sector.
 * PARANOIA_CB_DRIFT            : Error. Skipped sector.
 * PARANOIA_CB_BACKOFF          : Unsupported with current paranoia implementation. Should not occur.
 * PARANOIA_CB_OVERLAP          : No error. Dynamic overlap adjust. Sector does not seem to contain the current sector but the amount of overlapped data.
 * PARANOIA_CB_FIXUP_DROPPED    : Recoverable error. Fixed dropped bytes.
 * PARANOIA_CB_FIXUP_DUPED      : Recoverable error. Fixed duplicate bytes.
 * PARANOIA_CB_READERR          : Error. Reading error.
 * PARANOIA_CB_CACHEERR         : Cache error?
 * PARANOIA_CB_WROTE            : No error.
 * PARANOIA_CB_FINISHED         : No error. Just finished ripping.
 */
