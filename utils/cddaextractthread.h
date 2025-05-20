/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QMap>
#include <QString>
#include <QThread>

#include <KLocalizedString>

#include "utils/samplearray.h"

#include "cddaparanoia.h"

#define SECTOR_SIZE_BYTES 2352
#define SECTOR_SIZE_SAMPLES 588

class CDDAExtractThread : public QThread
{
    Q_OBJECT
public:
    CDDAExtractThread(CDDAParanoia *paranoia, QObject *parent = nullptr);
    ~CDDAExtractThread() override;

public Q_SLOTS:

    void start();
    void cancel();

    void enableParanoiaMode(const bool enable = true)
    {
        if (!p_paranoia)
            return;
        p_paranoia->enableParanoiaMode(enable);
    }
    bool paranoiaMode() const
    {
        if (!p_paranoia)
            return false;
        return p_paranoia->paranoiaModeEnabled();
    }
    void enableSkipReadErrors(const bool skip = true)
    {
        skip_read_errors = skip;
    }
    bool skipReadErrorsEnabled() const
    {
        return skip_read_errors;
    }
    void setParanoiaMaxRetriesOnReadError(int max_retries) // default: 20
    {
        if (!p_paranoia)
            return;
        p_paranoia->setParanoiaMaxRetriesOnReadError(max_retries);
    }
    int paranoiaMaxRetriesOnReadError() const
    {
        if (!p_paranoia)
            return -1;
        return p_paranoia->paranoiaMaxRetriesOnReadError();
    }

    void setSampleShift(const int shift)
    {
        sample_shift = shift;
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

    void setTimestampsIntoLog(const bool set)
    {
        set_timestamps_into_log = set;
    }

    void setTrackToRip(const track_t t)
    {
        track = t;
    } // if t==0 rip whole cd

    void skipTrack(const track_t t)
    {
        overall_sectors_read += p_paranoia->numOfFramesOfTrack(t);
    }

    bool isProcessing();

    const QStringList &log();

    void reset();

private Q_SLOTS:
    void slot_error(const QString &message, const QString &details);

Q_SIGNALS:
    void output(const QByteArray &);

    void progress(const int percent, const int sector, const int sectorsOverall);

    void error(const QString &message, const QString &details = QString());
    void warning(const QString &message);
    void info(const QString &message);

protected:
    void run() override;

private:
    CDDAParanoia *p_paranoia;
    bool skip_read_errors;

    unsigned long sectors_read;
    unsigned long overall_sectors_read;
    unsigned long sectors_all;

    long sector_start_pos;
    long prev_sector_pos;

    QMap<paranoia_cb_mode_t, int> paranoia_status_count;
    QMap<int, paranoia_cb_mode_t> paranoia_status_table;

    const QString paranoia_status_to_string(paranoia_cb_mode_t);
    const QString paranoia_status_table_to_string(const QMap<int, paranoia_cb_mode_t> &);

    int sample_shift;
    int sector_shift_left;
    int sector_shift_right;

    SampleArray silence;

    track_t track;

    bool b_first_run;
    bool b_interrupt;
    bool b_error;

    void append_log_line(const QString &line);
    QStringList p_log;
    bool set_timestamps_into_log;

    void create_status(long, paranoia_cb_mode_t);
    long status_previous_sector;

    // this friend function will call create_status(long, paranoia_cb_mode_t)
    friend void paranoia_callback(long, paranoia_cb_mode_t);
};
