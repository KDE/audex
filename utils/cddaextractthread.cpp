/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaextractthread.h"

#include <QDebug>

#include "utils/cddaparanoia.h"

static CDDAExtractThread *aet = nullptr;

void paranoia_callback(long sector, paranoia_cb_mode_t status)
{
    aet->create_status(sector, status);
}

CDDAExtractThread::CDDAExtractThread(CDDAParanoia *paranoia, QObject *parent)
    : QThread(parent)
{
    p_paranoia = paranoia;
    connect(p_paranoia, SIGNAL(error(const QString &, const QString &)), this, SLOT(slot_error(const QString &, const QString &)));

    reset();

    silence = SampleArray(SECTOR_SIZE_SAMPLES);

    // let the global paranoia callback have access to this to emit signals
    aet = this;
}

CDDAExtractThread::~CDDAExtractThread()
{
}

void CDDAExtractThread::start()
{
    QThread::start();
}

#define CHUNK_SIZE 58800 // in samples

void CDDAExtractThread::run()
{
    if (!p_paranoia)
        return;

    if (b_interrupt)
        return;

    b_interrupt = false;
    b_error = false;

    paranoia_status_count.clear();
    paranoia_status_table.clear();

    if (b_first_run) {
        qDebug() << p_paranoia->prettyTOC();
        p_log.append(i18n("TOC:"));
        p_log.append(p_paranoia->prettyTOC());
        b_first_run = false;
    }

    const int first_sector = p_paranoia->firstSectorOfTrack(track);
    const int last_sector = p_paranoia->lastSectorOfTrack(track);

    const int first_sector_of_disc = p_paranoia->firstSectorOfDisc();
    const int last_sector_of_disc = p_paranoia->lastSectorOfDisc();

    qDebug() << "Track:" << track;
    qDebug() << "First sector:" << first_sector;
    qDebug() << "Last sector:" << last_sector;

    qDebug() << "First sector of disc:" << first_sector_of_disc;
    qDebug() << "Last sector of disc:" << last_sector_of_disc;

    int start_sector = first_sector;
    int end_sector = last_sector;

    qDebug() << "Sample shift:" << sample_shift;
    qDebug() << "Sector shift left:" << sector_shift_left;
    qDebug() << "Sector shift right:" << sector_shift_right;

    p_log.append(i18n("Start ripping track %1 with length %2...", track, CDDAParanoia::LSN2MSF(last_sector - first_sector, QChar('-'))));
    p_log.append(i18n("Sample shift: %1", sample_shift));

    SampleArray chunk;
    bool overread = false;

    /* Set the start and the end sector
     *
     *           |Sector 0 |Sector 1 |Sector 2 |Sector 3 |Sector 4 |
     *           |*********|*********|*********|*********|*********|
     *
     * sample_shift == -3:
     * |Sektor-1 |Sector 0 |Sector 1 |Sector 2 |Sector 3 |Sector 4 |
     * |......***|*********|*********|*********|*********|******...|
     *
     * sample_shift == 3:
     *           |Sector 0 |Sector 1 |Sector 2 |Sector 3 |Sector 4 |Sector 5 |
     *           |...******|*********|*********|*********|*********|***......|
     *
     * "*": Samples to read, ".": samples to discard
     *
     * sample_shift == 0: If we do have no shift nothing will happen and start/end sector will be the same as the first and last sector of the track
     *
     * sample_shift < 0: If we have a shift to the left we must read sectors before the first sector recorded in the TOC for this track and may discard sectors
     * at the end
     *
     * sample_shift > 0: If we have a shift to the right we must read sectors after the last sector recorded in the TOC for this track and may discard sectors
     * at the beginning
     *
     * BUT if we cross the borders reported by firstSectorOfDisc()/lastSectorOfDisc() we fill up those areas with zero samples. Maybe in future we could check
     * if the drive supports some kind of overraeding feature into the lead in/out area.
     *
     */
    if (sample_shift < 0) {
        start_sector = first_sector - qAbs(sector_shift_left);
        // Check if we would read into lead in. If so, fill the chunk with zeros to avoid problems.
        // Maybe in future we would check if the drive supports reading of sectors of the lead in area.
        if (start_sector < first_sector_of_disc) {
            start_sector = first_sector_of_disc;
        } else {
            overread = true;
        }
        end_sector -= qAbs(sector_shift_right);
    } else if (sample_shift > 0) {
        start_sector = first_sector + qAbs(sector_shift_left);
        end_sector += qAbs(sector_shift_right);
        // Check if we would read into lead out. If so, fill the chunk **finally at the end** with zeros to avoid problems.
        // Maybe in future we would check if the drive supports reading of sectors of the lead out area.
        if (end_sector > last_sector_of_disc) {
            end_sector = last_sector_of_disc;
        } else {
            overread = true;
        }
    }

    sectors_all = end_sector - start_sector;
    sectors_read = 0;

    p_paranoia->paranoiaSeek(start_sector, SEEK_SET);

    // prepend chunk with null samples to replace out of border samples in case of a left shift
    // but only if we can't overread
    if (sample_shift < 0 && !overread)
        chunk.appendZeroSamples(qAbs(sample_shift));

    // do we have a full sector shift?
    const bool full_sector_shift = qAbs(sample_shift) % SECTOR_SIZE_SAMPLES == 0;

    QString paranoiaErrorMsg;

    // read the sectors
    for (int i = start_sector; i <= end_sector; ++i) {
        if (b_interrupt) {
            qDebug() << "Interrupt ripping";
            break;
        }

        SampleArray samples(p_paranoia->paranoiaRead(paranoia_callback), SECTOR_SIZE_SAMPLES * 2); // we do have two channels
        if (p_paranoia->paranoiaError(paranoiaErrorMsg)) {
            p_log.append(i18n("Error occured while reading sector %1 (track time pos %2): %3",
                              i,
                              CDDAParanoia::LSN2MSF(i - start_sector, QChar('-')),
                              paranoiaErrorMsg));
            if (!skip_read_errors) {
                if (track > 0)
                    Q_EMIT error(
                        i18n("An error occured while ripping track %1 at position %2. See log.", track, CDDAParanoia::LSN2MSF(i - start_sector, QChar('-'))));
                else
                    Q_EMIT error(i18n("An error occured while ripping at position %1. See log.", CDDAParanoia::LSN2MSF(i - start_sector, QChar('-'))));
                b_error = true;
                break;
            } else {
                if (track > 0)
                    Q_EMIT warning(
                        i18n("An error occured while ripping track %1 at position %2. See log.", track, CDDAParanoia::LSN2MSF(i - start_sector, QChar('-'))));
                else
                    Q_EMIT warning(i18n("An error occured while ripping at position %1. See log.", CDDAParanoia::LSN2MSF(i - start_sector, QChar('-'))));
            }
        }
        if (samples.isEmpty()) {
            if (paranoiaErrorMsg.isEmpty()) {
                p_log.append(i18n("Error reading sector %1 (track time pos %2)", i, CDDAParanoia::LSN2MSF(i - start_sector, QChar('-'))));
                if (!skip_read_errors)
                    Q_EMIT error(i18n("An error occured while ripping at position %1. See log.", CDDAParanoia::LSN2MSF(i - start_sector, QChar('-'))));
                else
                    Q_EMIT warning(i18n("An error occured while ripping at position %1. See log.", CDDAParanoia::LSN2MSF(i - start_sector, QChar('-'))));
            }
            if (!skip_read_errors) {
                b_error = true;
                break;
            }
            p_log.append(i18n("Error reading sector %1 (%2): **Filling whole sector with silence**", i, CDDAParanoia::LSN2MSF(i - start_sector, QChar('-'))));
            samples = silence;
        }

        if (sample_shift < 0 && overread && i == start_sector && !full_sector_shift) {
            // take only the fraction of right samples out of the sector
            chunk.append(samples.right(qAbs(sample_shift) % SECTOR_SIZE_SAMPLES));
        } else if (sample_shift < 0 && i == end_sector) {
            // take only the fraction of left samples out of the sector
            chunk.append(samples.left(SECTOR_SIZE_SAMPLES - (qAbs(sample_shift) % SECTOR_SIZE_SAMPLES)));
        } else if (sample_shift > 0 && i == start_sector) {
            // take only the fraction of right samples out of the sector
            chunk.append(samples.right(SECTOR_SIZE_SAMPLES - (qAbs(sample_shift) % SECTOR_SIZE_SAMPLES)));
        } else if (sample_shift > 0 && overread && i == end_sector) {
            // take only the fraction of left samples out of the sector
            chunk.append(samples.left(qAbs(sample_shift) % SECTOR_SIZE_SAMPLES));
        } else {
            chunk.append(samples);
        }

        if (chunk.size() >= CHUNK_SIZE) {
            Q_EMIT output(chunk.data());
            chunk.clear();
        }

        ++sectors_read;
        ++overall_sectors_read;
        float fraction = 0.0f;
        if (sectors_all > 0)
            fraction = (float)sectors_read / (float)sectors_all;
        Q_EMIT progress((int)(100.0f * fraction), i, overall_sectors_read);
    }

    // prepend chunk with null samples to replace out of border samples in case of a right shift
    // but only if we can't overread
    if (sample_shift > 0 && !overread)
        chunk.appendZeroSamples(qAbs(sample_shift));

    Q_EMIT output(chunk.data());

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
        p_log.append(i18n("Ripping of track %1 successfully finished", track));
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
    skip_read_errors = false;
    sample_shift = 0;
    sector_shift_left = 0;
    sector_shift_right = 0;
    track = 1;
    b_interrupt = false;
    b_error = false;
    status_previous_sector = -1;
    b_first_run = true;
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
