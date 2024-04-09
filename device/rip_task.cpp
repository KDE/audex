/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rip_task.h"

namespace Audex
{

namespace Device
{

Sector::Sector()
{
    p_data_crc32 = 0;
}

Sector::Sector(const QByteArray &data)
{
    p_data = data.left(SECTOR_SIZE_BYTES);
    p_data_crc32 = CRC::CRC32_Calculator::calc(p_data);
    if (data.size() == SECTOR_SIZE_BYTES + FORMATED_Q_SUBCHANNEL_SIZE_BYTES
            || data.size() == SECTOR_SIZE_BYTES + FORMATED_Q_SUBCHANNEL_SIZE_BYTES + C2_SIZE_BYTES)
        p_subchannel_data = data.mid(SECTOR_SIZE_BYTES, FORMATED_Q_SUBCHANNEL_SIZE_BYTES);
    if (data.size() == SECTOR_SIZE_BYTES + FORMATED_Q_SUBCHANNEL_SIZE_BYTES + C2_SIZE_BYTES)
        p_c2_err_ptrs = QBitArray::fromBits(data.mid(SECTOR_SIZE_BYTES + FORMATED_Q_SUBCHANNEL_SIZE_BYTES, C2_SIZE_BYTES).constData(), C2_SIZE_BYTES * 8);
}

Sector::Sector(const Sector &other)
{
    p_data = other.p_data;
    p_data_crc32 = other.p_data_crc32;
    p_subchannel_data = other.p_subchannel_data;
    p_c2_err_ptrs = other.p_c2_err_ptrs;
}

Sector &Sector::operator=(const Sector &other)
{
    p_data = other.p_data;
    p_data_crc32 = other.p_data_crc32;
    p_subchannel_data = other.p_subchannel_data;
    p_c2_err_ptrs = other.p_c2_err_ptrs;
    return *this;
}

bool Sector::operator==(const Sector &other) const
{
    return p_data_crc32 == other.p_data_crc32;
}

bool Sector::operator!=(const Sector &other) const
{
    return !(*this == other);
}

bool Sector::isEmpty() const
{
    return p_data.isEmpty();
}

const QByteArray &Sector::data() const
{
    return p_data;
}

const QByteArray *Sector::data()
{
    return &p_data;
}

char Sector::operator[](const int i)
{
    return p_data.at(i);
}

int Sector::subchannelAbsoluteSectornumber() const
{
    if (!p_subchannel_data.isEmpty() && (p_subchannel_data[0] & 0xFF) == 1) {
        quint8 min = SCSI::fromBCD8(static_cast<quint8>(p_subchannel_data[7]));
        quint8 sec = SCSI::fromBCD8(static_cast<quint8>(p_subchannel_data[8]));
        quint8 frames = SCSI::fromBCD8(static_cast<quint8>(p_subchannel_data[9]));
        return Toc::MSF2Frames(min, sec, frames);
    }
    return -1;
}

int Sector::subchannelTracknumber() const
{
    if (!p_subchannel_data.isEmpty() && (p_subchannel_data[0] & 0xFF) == 1)
        return SCSI::fromBCD8(static_cast<quint8>(p_subchannel_data[1]));
    return -1;
}

int Sector::subchannelIndex() const
{
    if (!p_subchannel_data.isEmpty() && (p_subchannel_data[0] & 0xFF) == 1)
        return SCSI::fromBCD8(static_cast<quint8>(p_subchannel_data[2]));
    return -1;
}

int Sector::checkC2ErrPtrs() const
{
    int result = 0;
    for (int i = 0; i < p_c2_err_ptrs.count(); ++i) {
        if (p_c2_err_ptrs.testBit(i))
            ++result;
    }
    return result;
}

bool Sector::checkC2ErrPtr(const int byteNo) const
{
    if (!p_c2_err_ptrs.isEmpty())
        return p_c2_err_ptrs.testBit(byteNo);
    else
        return true;
}

SectorArray::SectorArray(const int size)
{
    sectors = QVector<Sector>(size);
    sector_num_offset = 0;
}

void SectorArray::setSectorNumberOffset(const int offset)
{
    sector_num_offset = offset;
}

int SectorArray::sectorNumberOffset() const
{
    return sector_num_offset;
}

void SectorArray::append(const Sector &sector)
{
    sectors.append(sector);
}

const Sector &SectorArray::sector(const int sectorNum) const
{
    return sectors.at(sectorNum - sector_num_offset);
}

const Sector &SectorArray::at(const int i) const
{
    return sectors.at(i);
}

const QByteArray &SectorArray::fillData(QByteArray &data, const int sampleShift) const
{
    for (int i = 0; i < sectors.count(); ++i) {
        if (i == 0 && sampleShift < 0) {
            data.append(sectors.at(i).data().right((-sampleShift % SECTOR_SIZE_SAMPLES) * 4));
        } else if (i == 0 && sampleShift > 0) {
            data.append(sectors.at(i).data().right(SECTOR_SIZE_BYTES - ((sampleShift % SECTOR_SIZE_SAMPLES) * 4)));
        } else if (i == sectors.count() - 1 && sampleShift < 0) {
            data.append(sectors.at(i).data().left(SECTOR_SIZE_BYTES - ((-sampleShift % SECTOR_SIZE_SAMPLES) * 4)));
        } else if (i == sectors.count() - 1 && sampleShift > 0) {
            data.append(sectors.at(i).data().left((sampleShift % SECTOR_SIZE_SAMPLES) * 4));
        } else {
            data.append(sectors.at(i).data());
        }
    }
    return data;
}

QByteArray SectorArray::data(const int sampleShift)
{
    QByteArray result;
    fillData(result, sampleShift);
    return result;
}

int SectorArray::size() const
{
    return sectors.length();
}

int SectorArray::sizeBytes() const
{
    return sectors.length() * SECTOR_SIZE_BYTES;
}

void SectorArray::clear()
{
    sector_num_offset = 0;
    sectors.clear();
}

void SectorArray::chopleft(const int keepLastSectors)
{
    if (sectors.size() < keepLastSectors || keepLastSectors < 0)
        return;
    sectors.remove(0, sectors.size() - keepLastSectors);
    sector_num_offset += sectors.size() - keepLastSectors;
}

RipTask::RipTask(Drive *drive,
                 const int sampleOffset,
                 const bool overread,
                 const int numRetriesOnReadError,
                 const bool skipReadErrors,
                 const bool skipHardwareErrors,
                 const int bufferSize,
                 const bool subchannelSyncCheck,
                 const bool c2ErrPtrsCheck,
                 const bool calcChecksumCRC32,
                 const bool calcChecksumACR)
    : Task(drive->getUDI())
{
    this->drive = drive;

    set_sample_correction_offset(sampleOffset);

    this->overread = overread;

    num_retries_on_read_error = numRetriesOnReadError;
    skip_read_errors = skipReadErrors;
    skip_hardware_errors = skipHardwareErrors;
    buffer_size = bufferSize;
    subchannel_sync_check = subchannelSyncCheck;
    c2_err_ptrs_check = c2ErrPtrsCheck;
    calc_checksum_crc32 = calcChecksumCRC32;
    calc_checksum_acr = calcChecksumACR;

    p_error = false;

    rip_image = false;

    silence.fill(0, CD_FRAMESIZE_RAW);
}

void RipTask::setTracknumbersToRip(const TracknumberSet &tracknumbers)
{
    this->tracknumbers = tracknumbers;
    rip_image = tracknumbers.isEmpty();
}

TracknumberSet RipTask::tracknumbersToRip() const
{
    return tracknumbers;
}

void RipTask::run()
{
    if (isInterruptionRequested())
        return;

    Q_EMIT started(drive_udi);

    int device_handle = ::open(block_device.constData(), O_RDONLY | O_NONBLOCK);
    if (device_handle == -1) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to initialize device:" << block_device;
        log_entry(Message(i18n("Failed to initialize drive %1.", QString::fromLatin1(block_device)), Message::CRITICAL));
        Q_EMIT finished(drive_udi, false);
        return;
    }

    p_error = false;

    Toc::Toc toc = drive->cdInfo().toc();

    log_entry(Message(i18n("Drive Vendor: %1, Drive Model: %2, Drive Revision: %3",
                                     QString::fromLatin1(drive->driveInfo().getVendor()),
                                     QString::fromLatin1(drive->driveInfo().getModel()),
                                     QString::fromLatin1(drive->driveInfo().getRevision()))));
    log_entry(Message(i18n("TOC:")));
    log_entry(Message(toc.prettyTOC().join(QChar(u'\n'))));

    log_entry(Message(i18n("Correction sample offset: %1", sample_shift)));

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << ", Tracks:" << tracknumbers << ", Sample correction offset:" << sample_shift
             << ", Sector correction shift left:" << sector_shift_left << ", Sector correction shift right:" << sector_shift_right;

    int sectors_overall_count = toc.sectorCountTracknumbers(tracknumbers) + (tracknumbers.count() * (qAbs(sector_shift_left) + qAbs(sector_shift_right)));

    QList<int> tracknumberlist = tracknumbers.values();
    if (rip_image)
        tracknumberlist.append(0);
    else
        std::sort(tracknumberlist.begin(), tracknumberlist.end());

    SectorArray buffer(buffer_size);

    int sectors_read = 0;
    int sectors_read_overall = 0;

    Checksum32Map checksums_crc32;
    Checksum32Map checksums_acr;
    Checksum32Map checksums_acr_v2;

    int prev_track = -1;
    for (int i = 0; i < tracknumberlist.count(); ++i) {
        if (isInterruptionRequested()) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "ripping stopped";
            break;
        }

        int track = tracknumberlist.at(i);
        if (track == 0 && tracknumberlist.count() > 1)
            continue;
        if (track != 0 && !toc.isAudioTrack(track))
            continue;

        if (!rip_image)
            Q_EMIT nextTrack(drive_udi, prev_track, track);

        if (track == 0) {
            first_sector = toc.firstSectorOfDisc() + sector_shift_left;
            last_sector = toc.lastSectorOfLastAudioTrack() + sector_shift_right;
        } else {
            first_sector = toc.firstSectorOfTrack(track) + sector_shift_left;
            last_sector = toc.lastSectorOfTrack(track) + sector_shift_right;
        }

        if (!overread) {
            if (first_sector < toc.firstSectorOfDisc()) {
                // if we do not overread fill the sectors of the lead-in with silence
                for (int i = 0; i < toc.firstSectorOfDisc() - first_sector; ++i)
                    buffer.append(Sector(silence));
                first_sector = toc.firstSectorOfDisc();
            }
            if (last_sector > toc.lastSectorOfLastAudioTrack())
                last_sector = toc.lastSectorOfLastAudioTrack();
        }

        qDebug() << "Track:" << track << "First sector:" << first_sector << "Last sector:" << last_sector;

        int sector_count = last_sector - first_sector;

        sectors_read = 0;

        if (track == prev_track && sample_shift) {
            buffer.chopleft(1);
        } else {
            current_sector = first_sector;
            buffer.clear();
            buffer.setSectorNumberOffset(current_sector);
        }

        QString min = QStringLiteral(u"%1").arg((last_sector - first_sector / SECTORS_PER_SECOND) / 60, 2, 10, QChar(u'0'));
        QString sec = QStringLiteral(u"%1").arg((last_sector - first_sector / SECTORS_PER_SECOND) % 60, 2, 10, QChar(u'0'));

        if (!rip_image) {
            log_entry(Message(i18n("Ripping track %1 (%2:%3)...", track, min, sec)));
        } else {
            log_entry(Message(i18n("Ripping all tracks at once (%1:%2)...", min, sec)));
        }

        log_entry(Message(i18n("First sector: %1, Last sector: %2.", first_sector, last_sector)));

        if (calc_checksum_crc32)
            crc32_calculator.reset();

        if (calc_checksum_acr)
            acr_checksum_calculator.reset();

        QElapsedTimer speed_timer;
        speed_timer.start();
        qreal current_speed;
        int last_checkpoint = 0;

        SCSI::ErrorCode ec;
        int buffers_read = 0;
        while (current_sector <= last_sector) {
            if (isInterruptionRequested()) {
                qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "ripping stopped";
                break;
            }

            // Complete reset buffer
            // Downside: You must read at least one buffer twice
            // if (sample_shift > 0 && current_sector > first_sector) {
            //     current_sector -= sector_shift_right;
            // }
            // buffer.clear();
            // buffer.setSectorNumberOffset(current_sector);

            if (sample_shift) {
                buffer.chopleft(1);
            } else {
                buffer.clear();
                buffer.setSectorNumberOffset(current_sector);
            }

            while (buffer.size() < buffer_size && current_sector <= last_sector) {
                if (isInterruptionRequested()) {
                    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "ripping stopped";
                    break;
                }

                buffer.append(read_current_sector(device_handle));

                ++current_sector;

                ++sectors_read;
                ++sectors_read_overall;
                qreal fraction = 0.0f;
                if (sectors_overall_count > 0)
                    fraction = (qreal)sectors_read / (qreal)sector_count;
                qreal fraction_overall = 0.0f;
                if (sectors_overall_count > 0)
                    fraction_overall = (qreal)sectors_read_overall / (qreal)sectors_overall_count;

                if (sectors_read >= last_checkpoint + 75) {
                    current_speed = (40.0f / 3.0f) * (float)(sectors_read - last_checkpoint) / (float)speed_timer.elapsed();
                    last_checkpoint = sectors_read;
                    speed_timer.restart();
                }

                Q_EMIT progress(drive_udi, track, fraction, fraction_overall, current_sector, sectors_read_overall, current_speed);
            }

            ++buffers_read;

            // if we do not overread, fill the sectors of the lead-out with silence
            if (current_sector >= toc.lastSectorOfLastAudioTrack() && !overread && sector_shift_right > 0)
                for (int i = 0; i < sector_shift_right; ++i)
                    buffer.append(Sector(silence));

            Q_EMIT output(drive_udi, buffer.data(sample_shift));

            if (calc_checksum_crc32)
                crc32_calculator.process(buffer.data(sample_shift));

            if (calc_checksum_acr)
                acr_checksum_calculator.process(buffer.data(sample_shift), toc.isFirstTrack(track) && buffers_read == 1 ? 5 * CD_SECTORSIZE_SAMPLES - 1 : 0, toc.isLastTrack(track) && current_sector == last_sector ? 5 * CD_SECTORSIZE_SAMPLES - 1 : 0);

        }

        if (calc_checksum_crc32) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "CRC32 Track" << track << ":" << QStringLiteral(u"%1").arg(crc32_calculator.result(), 8, 16, QChar(u'0'));
            checksums_crc32.insert(track, crc32_calculator.result());
        }

        if (calc_checksum_acr) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "ACR Track" << track << ":" << QStringLiteral(u"%1").arg(acr_checksum_calculator.result(), 8, 16, QChar(u'0'));
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "ACR v2 Track" << track << ":" << QStringLiteral(u"%1").arg(acr_checksum_calculator.resultV2(), 8, 16, QChar(u'0'));
            checksums_acr.insert(track, acr_checksum_calculator.result());
            checksums_acr_v2.insert(track, acr_checksum_calculator.resultV2());
        }

        if (p_error) {
            log_entry(Message(i18n("Ripping was canceled due to an error.")));
        } else if (isInterruptionRequested()) {
            log_entry(Message(i18n("Ripping interrupted due to user cancel.")));
        } else {
            if (!rip_image) {
                log_entry(Message(i18n("Ripping OK (Track %1).", track)));
            } else {
                log_entry(Message(i18n("Ripping OK.")));
            }
        }

        prev_track = track;
    }

    Q_EMIT nextTrack(drive_udi, prev_track, -1);

    if (!p_error)
        log_entry(Message(i18n("Ripping finished.")));

    ::close(device_handle);

    Q_EMIT finished(drive_udi, !p_error && !isInterruptionRequested(), checksums_crc32, checksums_acr, checksums_acr_v2);
}

Sector RipTask::read_current_sector(const int device_handle, int attempt_num)
{
    SCSI::ErrorCode ec;

    QByteArray buf = SCSI::readCD(device_handle, ec, current_sector, 1, c2_err_ptrs_check ? 0x01 : 0x00, subchannel_sync_check ? 0x02 : 0x00);

    if (ec.senseKey() == 0x01) {
        log_entry(Message(i18n("Reading sector %1 (track time pos %2) recovered data.", current_sector, Toc::Frames2TimeString(current_sector - first_sector)),
                                    Message::INFO));
    } else if (ec.senseKey() == 0x03) {
        log_entry(Message(
                     i18n("Reading error occured while reading sector %1 (track time pos %2).", current_sector, Toc::Frames2TimeString(current_sector - first_sector)),
                     Message::ERROR,
                     ec.errorCode(),
                     ec.senseKeyString()));

        if (attempt_num < num_retries_on_read_error) {
            log_entry(Message(i18n("Starting next attempt #%1 to read sector", attempt_num + 1, current_sector), Message::INFO));
            return read_current_sector(device_handle, attempt_num + 1);
        } else if (!skip_read_errors) {
            requestInterruption();
            return Sector();
        }

        log_entry(Message(i18n("There is probably an audible defect at sector %1 (track time pos %2).",
                                         current_sector,
                                         Toc::Frames2TimeString(current_sector - first_sector)),
                                    Message::ERROR));
    } else if (buf.isEmpty()) {
        log_entry(Message(
                     i18n("No data retrieved while reading sector %1 (track time pos %2).", current_sector, Toc::Frames2TimeString(current_sector - first_sector)),
                     Message::ERROR));
        if (!skip_read_errors) {
            requestInterruption();
            return Sector();
        }

        if (attempt_num < num_retries_on_read_error) {
            log_entry(Message(i18n("Starting next attempt #%1 to read sector", attempt_num + 1, current_sector), Message::INFO));
            return read_current_sector(device_handle, attempt_num + 1);
        } else if (!skip_read_errors) {
            requestInterruption();
            return Sector();
        }

        log_entry(Message(i18n("Filling sector %1 (track time pos %2) with silence.", current_sector, Toc::Frames2TimeString(current_sector - first_sector)),
                                    Message::ERROR));
        buf = silence;
    } else if (ec) {
        log_entry(Message(
                     i18n("Hardware error occured while reading sector %1 (track time pos %2).", current_sector, Toc::Frames2TimeString(current_sector - first_sector)),
                     Message::CRITICAL,
                     ec.errorCode(),
                     ec.senseKeyString()));

        if (!skip_hardware_errors) {
            requestInterruption();
            return Sector();
        }

        log_entry(Message(i18n("There is probably an audible defect at sector %1 (track time pos %2).",
                                         current_sector,
                                         Toc::Frames2TimeString(current_sector - first_sector)),
                                    Message::ERROR));
    }

    if (attempt_num > 0) {
        log_entry(Message(i18n("Reading sector %1 (track time pos %2) succeeded.", current_sector, Toc::Frames2TimeString(current_sector - first_sector)),
                                    Message::INFO));
    }

    Sector sector(buf);

    if (!ec && !buf.isEmpty()) {
        if (subchannel_sync_check && sector.subchannelAbsoluteSectornumber() != current_sector + 150) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Sector sequence out of sync with subchannel data:" << current_sector;
            log_entry(Message(i18n("Sector number %1 (track time pos %2) out of sync with subchannel. There might be an audible defect.",
                                             current_sector,
                                             Toc::Frames2TimeString(current_sector - first_sector)),
                                        Message::WARNING));
        }

        if (c2_err_ptrs_check && sector.checkC2ErrPtrs()) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Sector reports" << current_sector << "C2 errors";
            log_entry(Message(i18n("Sector number %1 (track time pos %2) reports %3 C2 errors. There might be an audible defect.",
                                             current_sector,
                                             Toc::Frames2TimeString(current_sector - first_sector),
                                             sector.checkC2ErrPtrs()),
                                        Message::WARNING));
        }
    }

    return sector;
}

void RipTask::set_sample_correction_offset(const int offset)
{
    sample_shift = offset;
    sector_shift_left = 0;
    sector_shift_right = 0;
    // How many full sectors we do need to read at the beginning (left) and end (right) of the stream?
    if (sample_shift > 0) {
        sector_shift_left = sample_shift / SECTOR_SIZE_SAMPLES;
        sector_shift_right = sector_shift_left + 1;
    } else if (sample_shift < 0) {
        sector_shift_right = sample_shift / SECTOR_SIZE_SAMPLES;
        sector_shift_left = sector_shift_right - 1;
    }
}

}

}
