/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaripthread.h"

#include <QDateTime>
#include <QDebug>

#include <cdio/paranoia/paranoia.h>

namespace Audex
{

static CDDARipThread *aet = nullptr;

void paranoia_callback(long sector, paranoia_cb_mode_t status)
{
    aet->create_status(sector, status);
}

CDDARipThread::CDDARipThread(const QByteArray &blockDevice, const Toc::Toc toc, QObject *parent)
    : QThread(parent)
{
    p_toc = toc;

    p_paranoia = new Device::Cdparanoia(blockDevice);
    if (!p_paranoia) {
        qDebug() << "Unable to create paranoia object.";
        Q_EMIT error(i18n("Unable to create paranoia object."), i18n("This is an internal error. Check your hardware. If all okay please make bug report."));
        return;
    }

    reset();

    silence = SampleArray(SECTOR_SIZE_SAMPLES);

    set_timestamps_into_log = false;

    // let the global paranoia callback have access to this to emit signals
    aet = this;
}

CDDARipThread::~CDDARipThread()
{
    if (p_paranoia)
        delete p_paranoia;
}

void CDDARipThread::start()
{
    QThread::start();
}

#define CHUNK_SIZE 58800 // in samples

void CDDARipThread::run()
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
        qDebug() << p_toc.prettyTOC();
        append_log_line(p_toc.prettyTOC().join(QChar('\n')));
        append_log_line(i18n("Sample shift: %1", sample_shift));
        if (sample_shift) {
            Q_EMIT info(i18n("Ripping with shift of %1 samples", sample_shift));
        }
        if (p_paranoia->paranoiaModeEnabled()) {
            append_log_line(i18n("Paranoia mode enabled"));
        } else {
            append_log_line(i18n("Paranoia mode disabled"));
        }
        b_first_run = false;
    }

    int first_sector = p_toc.firstSectorOfTrack(track);
    int last_sector = p_toc.lastSectorOfTrack(track);

    const int first_sector_of_disc = p_toc.firstSectorOfDisc();
    const int last_sector_of_disc = p_toc.lastSectorOfDisc();

    if (track == 0) {
        first_sector = first_sector_of_disc;
        last_sector = last_sector_of_disc;
    }

    qDebug() << "Track:" << track;
    qDebug() << "First sector:" << first_sector;
    qDebug() << "Last sector:" << last_sector;

    qDebug() << "First sector of disc:" << first_sector_of_disc;
    qDebug() << "Last sector of disc:" << last_sector_of_disc;

    int start_sector = first_sector;
    int end_sector = last_sector;

    sector_start_pos = start_sector;

    qDebug() << "Sample shift:" << sample_shift;
    qDebug() << "Sector shift left:" << sector_shift_left;
    qDebug() << "Sector shift right:" << sector_shift_right;

    if (track > 0) {
        append_log_line(i18n("Start ripping track %1...", track));
        Q_EMIT info(i18n("Start ripping track %1...", track));
    } else {
        append_log_line(i18n("Start ripping whole cd..."));
        Q_EMIT info(i18n("Start ripping whole cd..."));
    }

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
            append_log_line(
                i18n("Error occured while reading sector %1 (track time pos %2): %3", i, Toc::Frames2MSFString(i - start_sector), paranoiaErrorMsg));
            if (!skip_read_errors) {
                if (track > 0)
                    Q_EMIT error(i18n("An error occured while ripping track %1 at position %2. See log.", track, Toc::Frames2MSFString(i - start_sector)));
                else
                    Q_EMIT error(i18n("An error occured while ripping at position %1. See log.", Toc::Frames2MSFString(i - start_sector)));
                b_error = true;
                break;
            } else {
                if (track > 0)
                    Q_EMIT warning(i18n("An error occured while ripping track %1 at position %2. See log.", track, Toc::Frames2MSFString(i - start_sector)));
                else
                    Q_EMIT warning(i18n("An error occured while ripping at position %1. See log.", Toc::Frames2MSFString(i - start_sector)));
            }
        }
        if (samples.isEmpty()) {
            if (paranoiaErrorMsg.isEmpty()) {
                append_log_line(i18n("Error reading sector %1 (track time pos %2)", i, Toc::Frames2MSFString(i - start_sector)));
                if (!skip_read_errors)
                    Q_EMIT error(i18n("An error occured while ripping at position %1. See log.", Toc::Frames2MSFString(i - start_sector)));
                else
                    Q_EMIT warning(i18n("An error occured while ripping at position %1. See log.", Toc::Frames2MSFString(i - start_sector)));
            }
            if (!skip_read_errors) {
                b_error = true;
                break;
            }
            append_log_line(i18n("Error reading sector %1 (%2): **Filling whole sector with silence**", i, Toc::Frames2MSFString(i - start_sector)));
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
    }

    // prepend chunk with null samples to replace out of border samples in case of a right shift
    // but only if we can't overread
    if (sample_shift > 0 && !overread)
        chunk.appendZeroSamples(qAbs(sample_shift));

    Q_EMIT output(chunk.data());

    if (p_paranoia->paranoiaModeEnabled()) {
        if (track > 0) {
            append_log_line(i18n("Paranoia status report for track %1:", track));
        } else {
            append_log_line(i18n("Paranoia status report:"));
        }
        append_log_line(paranoia_status_table_to_string(paranoia_status_table));
    }

    if (b_error) {
        Q_EMIT error(i18n("Ripping was canceled due to an error."));
        append_log_line(i18n("Ripping was canceled due to an error."));
    } else if (b_interrupt) {
        Q_EMIT error(i18n("User canceled extracting."));
        append_log_line(i18n("Extraction interrupted"));
    } else {
        if (track > 0) {
            Q_EMIT info(i18n("Ripping OK (Track %1).", track));
            append_log_line(i18n("Ripping of track %2 successfully finished"));
        } else {
            Q_EMIT info(i18n("Ripping OK."));
            append_log_line(i18n("Ripping successfully finished"));
        }
    }
}

void CDDARipThread::cancel()
{
    b_interrupt = true;
}

bool CDDARipThread::isProcessing()
{
    return !(b_interrupt || !isRunning());
}

const QStringList &CDDARipThread::log()
{
    return p_log;
}

void CDDARipThread::reset()
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
    prev_sector_pos = 0;
    sector_start_pos = 0;
}

void CDDARipThread::slot_error(const QString &message, const QString &details)
{
    Q_EMIT error(message, details);
}

const QString CDDARipThread::paranoia_status_to_string(paranoia_cb_mode_t status)
{
    switch (status) {
    case PARANOIA_CB_READ:
        return "READ: No error";
    case PARANOIA_CB_VERIFY:
        return "Verifying jitter";
    case PARANOIA_CB_FIXUP_EDGE:
        return "Fixed edge jitter";
    case PARANOIA_CB_FIXUP_ATOM:
        return "Fixed atom jitter";
    case PARANOIA_CB_SKIP:
        return "ERROR: Sector skipped";
    case PARANOIA_CB_DRIFT:
        return "DRIFT ERROR: Sector skipped";
    case PARANOIA_CB_OVERLAP:
        return "Dynamic overlap adjust";
    case PARANOIA_CB_FIXUP_DROPPED:
        return "Fixed dropped bytes";
    case PARANOIA_CB_FIXUP_DUPED:
        return "Fixed duplicate bytes";
    case PARANOIA_CB_READERR:
        return "ERROR: Read error";
    case PARANOIA_CB_CACHEERR:
        return "ERROR: Cache error";
    default:
        return "UNKNOWN Paranoia Status";
    }
}

const QString CDDARipThread::paranoia_status_table_to_string(const QMap<int, paranoia_cb_mode_t> &status_table)
{
    QString result;
    for (auto [key, value] : status_table.asKeyValueRange()) {
        result += QString("Sector %1: %2\n").arg(key).arg(paranoia_status_to_string(value));
    }
    result.chop(1);
    return result;
}

void CDDARipThread::append_log_line(const QString &line)
{
    if (set_timestamps_into_log) {
        p_log.append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString(Qt::RFC2822Date)).arg(line));
    } else {
        p_log.append(line);
    }
}

void CDDARipThread::create_status(long in_pos, paranoia_cb_mode_t status)
{
    const qint32 sector_pos = (in_pos * 2) / SECTOR_SIZE_BYTES;

    if (status == PARANOIA_CB_READ && sector_pos > prev_sector_pos) {
        // qDebug() << "Current sector reading position:" << sector_pos;
        const int percent = (int)(100.0f * (float)(sector_pos - sector_start_pos) / (float)sectors_all);
        Q_EMIT progress(percent, sectors_read, overall_sectors_read);
        prev_sector_pos = sector_pos;
    }

    if (status == PARANOIA_CB_READ || status == PARANOIA_CB_FINISHED || status == PARANOIA_CB_WROTE || status == PARANOIA_CB_OVERLAP)
        return;

    paranoia_status_count[status]++;
    paranoia_status_table.insert(sector_pos, status);
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
 * PARANOIA_CB_READERR          : Error. Read error.
 * PARANOIA_CB_CACHEERR         : Cache error?
 * PARANOIA_CB_WROTE            : No error.
 * PARANOIA_CB_FINISHED         : No error. Just finished ripping.
 */

}
