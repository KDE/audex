/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RIP_TASK_H
#define RIP_TASK_H

#include <QtMath>

#include <QBitArray>
#include <QPair>
#include <QSet>
#include <QString>
#include <QThread>
#include <QElapsedTimer>

#include <KLocalizedString>

extern "C" {
#include <unistd.h>
}

#include "datatypes/cdinfo.h"
#include "datatypes/driveinfo.h"
#include "datatypes/message.h"
#include "datatypes/toc.h"
#include "device/scsi.h"
#include "utils/accuraterip.h"
#include "utils/crc.h"

#include "task.h"

namespace Audex
{

namespace Device
{

class Sector
{
public:
    explicit Sector();
    explicit Sector(const QByteArray &data);
    Sector(const Sector &other);
    Sector &operator=(const Sector &other);

    bool operator==(const Sector &other) const;
    bool operator!=(const Sector &other) const;

    bool isEmpty() const;

    const QByteArray &data() const;
    const QByteArray *data();
    char operator[](const int i);

    int subchannelAbsoluteSectornumber() const;
    int subchannelTracknumber() const;
    int subchannelIndex() const;

    int checkC2ErrPtrs() const;
    bool checkC2ErrPtr(const int byteNo) const;

private:
    QByteArray p_data;
    quint32 p_data_crc32;
    QByteArray p_subchannel_data;
    QBitArray p_c2_err_ptrs;
};

class SectorArray
{
public:
    explicit SectorArray(const int size = 0);

    void setSectorNumberOffset(const int offset);
    int sectorNumberOffset() const;

    void append(const Sector &sector);

    const Sector &sector(const int sectorNum) const;
    const Sector &at(const int i) const;

    const QByteArray &fillData(QByteArray &data, const int sampleShift = 0) const;
    QByteArray data(const int sampleShift = 0);

    int size() const;
    int sizeBytes() const;

    void clear();
    void chopleft(const int keepLastSectors = 0);

private:
    QVector<Sector> sectors;
    int sector_num_offset;
};

class RipTask : public Task
{
    Q_OBJECT

public:
    enum ReadMode {
        Normal = 0, // read sectors once
        Secure, // read buffer multiple times
    };

    RipTask(Drive *drive,
            const int sampleOffset = 0,
            const bool overread = false,
            const int numRetriesOnReadError = 0,
            const bool skipReadErrors = true,
            const bool skipHardwareErrors = false,
            const int bufferSize = 3200, // in sectors, so multiply with 2352 to get size in bytes
            const bool subchannelSyncCheck = false, // very slow
            const bool c2ErrPtrsCheck = false,
            const bool calcChecksumCRC32 = true,
            const bool calcChecksumACR = true);

    // if trackset is empty rip whole cd at once
    void setTracknumbersToRip(const TracknumberSet &tracks = TracknumberSet());
    TracknumberSet tracknumbersToRip() const;

Q_SIGNALS:
    void output(const QString &driveUDI, const QByteArray &);

    void progress(const QString &driveUDI, const int tracknumber, const qreal fractionCurrentTrack, const qreal fraction, const int currentSector, const int sectorsRead, const qreal currentSpeed);
    void nextTrack(const QString &driveUDI, const int prevTracknumber, const int tracknumber);

    void finished(const QString& driveUDI, const bool successful, const Checksum32Map& checksumsCRC32 = Checksum32Map(), const Checksum32Map& checksumsACR = Checksum32Map(), const Checksum32Map& checksumsACRV2 = Checksum32Map());

protected:
    void run() override;

private:
    Drive *drive;

    ReadMode read_mode;
    int buffer_size; // should be at least 8 MiB
    int num_retries_on_read_error;
    bool skip_read_errors;
    bool skip_hardware_errors;
    bool subchannel_sync_check;
    bool c2_err_ptrs_check;
    bool calc_checksum_crc32;
    bool calc_checksum_acr;

    TracknumberSet tracknumbers;
    bool rip_image; // rip whole cd as single track file

    Sector read_current_sector(const int device_handle, int attempt_num = 0);
    int first_sector;
    int current_sector;
    int last_sector;
    int current_track_index;

    QByteArray silence;

    bool overread;

    void set_sample_correction_offset(const int offset);
    int sample_shift;
    int sector_shift_left;
    int sector_shift_right;

    AccurateRip::ChecksumCalculator acr_checksum_calculator;
    CRC::CRC32_PCM_Calculator crc32_calculator;

    bool p_error;
};

}

}

#endif
