/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CDDAEXTRACTTHREAD_HEADER
#define CDDAEXTRACTTHREAD_HEADER

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
    void setSampleOffset(const int offset)
    {
        sample_offset = offset;
    }
    void setTrackToRip(const unsigned int t)
    {
        track = t;
    } // if t==0 rip whole cd
    void skipTrack(const unsigned int t)
    {
        overall_sectors_read += p_cdio->numOfFramesOfTrack(t);
    }

    bool isProcessing();

    const QStringList &protocol();

    CDDACDIO *cdio()
    {
        return p_cdio;
    }

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
    int sample_offset;
    bool sample_offset_done;

    unsigned int track;

    bool b_interrupt;
    bool b_error;

    QStringList extract_protocol;

    void createStatus(long, paranoia_cb_mode_t);

    // status variables
    long overlap;
    long read_sectors;

    bool read_error;
    bool scratch_detected;

    // this friend function will call createStatus(long, paranoia_cb_mode_t)
    friend void paranoiaCallback(long, paranoia_cb_mode_t);
};

#endif
