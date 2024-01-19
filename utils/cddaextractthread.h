/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CDDAEXTRACTTHREAD_HEADER
#define CDDAEXTRACTTHREAD_HEADER

#include <QHash>
#include <QString>
#include <QThread>

#include <KLocalizedString>

#include "cddacdio.h"

class CDDAExtractThread : public QThread
{
    Q_OBJECT
public:
    CDDAExtractThread(QObject *parent, CDDACDIO *cdio);
    ~CDDAExtractThread() override;

public Q_SLOTS:
    void start();
    void cancel();
    void setParanoiaFullMode(const bool mode)
    {
        paranoia_full_mode = mode;
    }
    void setParanoiaMaxRetries(const int max_retries)
    {
        paranoia_retries = max_retries;
    }
    void setParanoiaNeverSkip(const bool never_skip)
    {
        paranoia_never_skip = never_skip;
    }
    void setSkipReadingErrors(const bool skip_reading_errors)
    {
        this->skip_reading_errors = skip_reading_errors;
    }
    int setSampleOffset(const int offset)
    {
        if (offset == 0) {
            sample_offset = 0;
            sector_offset = 0;
        }

        sample_offset = offset;

        // How many full sectors are sample offset?
        sector_offset = sample_offset / CD_FRAMESIZE_SAMPLES;
        sample_offset_fraction = sample_offset % CD_FRAMESIZE_SAMPLES;

        if (sample_offset < 0) {
            sample_offset_fraction -= CD_FRAMESIZE_SAMPLES;
            --sector_offset;
        }

        return 0;
    }
    void setTrackToRip(const track_t t)
    {
        track = t;
    } // if t==0 rip whole cd
    void skipTrack(const track_t t)
    {
        overall_sectors_read += p_cdio->numOfFramesOfTrack(t);
    }

    bool isProcessing();

    const QStringList &log();

    CDDACDIO *cdio()
    {
        return p_cdio;
    }

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
    /** reimplemented from QThread. Does the work */
    void run() override;

private:
    CDDACDIO *p_cdio;

    long first_sector;
    long current_sector;
    long last_sector;
    unsigned long sectors_read;
    unsigned long overall_sectors_read;
    unsigned long sectors_all;

    bool paranoia_full_mode;
    int paranoia_retries;
    bool paranoia_never_skip;
    bool skip_reading_errors;
    QMap<paranoia_cb_mode_t, int> paranoia_status_count;
    QHash<int, paranoia_cb_mode_t> paranoia_status_table;

    QByteArray silence;

    int sample_offset;
    int sector_offset;
    int sample_offset_fraction;
    QByteArray sample_offset_fraction_buffer;

    track_t track;

    bool b_first_run;
    bool b_interrupt;
    bool b_error;

    QStringList p_log;

    void create_status(long, paranoia_cb_mode_t);
    long status_previous_sector;

    // this friend function will call create_status(long, paranoia_cb_mode_t)
    friend void paranoia_callback(long, paranoia_cb_mode_t);
};

#endif
