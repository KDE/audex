/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AUDEX_H
#define AUDEX_H

#include <QDir>
#include <QQueue>
#include <QStorageInfo>
#include <QString>
#include <QTemporaryDir>
#include <QTimer>

#include <KLocalizedString>

#include "models/cdinfomodel.h"
#include "models/profilemodel.h"
#include "datatypes/parameters.h"
#include "device/manager.h"
#include "utils/cuesheet_writer.h"
#include "utils/encoderwrapper.h"
#include "utils/hashlist.h"
#include "utils/playlist.h"
#include "utils/schemeparser.h"
#include "utils/upload.h"
#include "utils/wavefile_writer.h"

#include "preferences.h"

namespace Audex
{

class Job : public QObject
{
    Q_OBJECT
public:
    explicit Job(QObject *parent = nullptr)
        : QObject(parent)
    {
        track_number = 0;
    }
    ~Job() override
    {
    }
    void setSourceFilename(const QString &n)
    {
        source_filename = n;
    }
    QString sourceFilename() const
    {
        return source_filename;
    }
    void setTargetFilename(const QString &n)
    {
        target_filename = n;
    }
    QString targetFilename() const
    {
        return target_filename;
    }
    void setTrackNumber(const int t)
    {
        track_number = t;
    }
    int trackNumber() const
    {
        return track_number;
    }

private:
    QString source_filename;
    QString target_filename;
    int track_number;
};

class Jobs : public QObject
{
    Q_OBJECT
public:
    explicit Jobs(QObject *parent = nullptr)
        : QObject(parent)
    {
        job_in_progress = false;
    }
    ~Jobs() override
    {
        for (int i = 0; i < cache.count(); i++) {
            // make sure all files are absent
            QFile file(cache.at(i)->sourceFilename());
            if (file.exists())
                file.remove();
            delete cache.at(i);
        }
    }
    Job *orderJob()
    {
        if (job_queue.isEmpty()) {
            return nullptr;
        } else {
            job_in_progress = true;
            return job_queue.dequeue();
        }
    }
    void reportJobFinished()
    {
        job_in_progress = false;
    }
    bool jobInProgress()
    {
        return job_in_progress;
    }
    bool pendingJobs()
    {
        return (job_queue.count() > 0);
    }

public Q_SLOTS:
    void addNewJob(const QString &sourceFilename, const QString &targetFilename, const int trackNumber)
    {
        auto *j = new Job();
        j->setSourceFilename(sourceFilename);
        j->setTargetFilename(targetFilename);
        j->setTrackNumber(trackNumber);
        job_queue.enqueue(j);
        cache.append(j);
        Q_EMIT newJobAvailable();
    }

Q_SIGNALS:
    void newJobAvailable();

private:
    QQueue<Job *> job_queue;
    QList<Job *> cache;
    bool job_in_progress;
};

class Audex : public QObject
{
    Q_OBJECT

public:
    Audex(QWidget *parent, ProfileModel *profile_model, Device::Manager *manager);
    ~Audex() override;

    bool prepare(const QString& udi, const TracknumberSet& tracksToRip);

public Q_SLOTS:
    void start();
    void cancel();

    const QStringList &ripLog();
    const QStringList &encoderLog();

private Q_SLOTS:
    void start_rip();
    void rip_next_track(const QString &driveUDI, const int prev_tracknumber, const int tracknumber);
    void start_encode();
    void finish_encode();

    void calculate_speed_encode();

    void progress_rip(const QString &driveUDI,
                         const int tracknumber,
                         const qreal fractionCurrentTrack,
                         const qreal fraction,
                         const int currentSector,
                         const int sectorsRead,
                         const qreal currentSpeed);
    void progress_encode(int percent);

    void write_to_wave(const QString &drive_udi, const QByteArray &data);

    void slot_error(const QString &message, const QString &details = QString());
    void slot_warning(const QString &message);
    void slot_info(const QString &message);

    void check_if_thread_still_running();

Q_SIGNALS:
    void changedExtractTrack(int no, int total, const QString &artist, const QString &title);
    void changedEncodeTrack(int no, int total, const QString &filename);

    void progressExtractTrack(int percent);
    void progressExtractOverall(int percent);
    void progressEncodeTrack(int percent);
    void progressEncodeOverall(int percent);

    void speedExtract(qreal times);
    void speedEncode(qreal times);

    void finished(bool successful);

    void timeout();

    void error(const QString &message, const QString &details = QString());
    void warning(const QString &message);
    void info(const QString &message);

private:
    QWidget *parent;
    ProfileModel *profile_model;
    Device::Manager *manager;
    EncoderWrapper *encoder_wrapper;
    Jobs *jobs;
    WAVEFileWriter *wave_file_writer;
    QTemporaryDir tmp_dir;

    QString udi;
    TracknumberSet tracks_to_rip;

    QString profile_name;
    QString suffix;
    bool single_file;

    bool construct_target_filename(QString &targetFilename,
                                   int trackno,
                                   int cdno,
                                   int nooftracks,
                                   int gindex,
                                   const QString &artist,
                                   const QString &title,
                                   const QString &tartist,
                                   const QString &ttitle,
                                   const QString &date,
                                   const QString &genre,
                                   const QString &isrc,
                                   const QString &ext,
                                   const QString &basepath,
                                   bool fat32_compatible,
                                   bool replacespaceswithunderscores,
                                   bool _2digitstracknum,
                                   bool overwrite_existing_files,
                                   bool is_first_track);

    bool construct_target_filename_for_singlefile(QString &targetFilename,
                                                  int cdno,
                                                  int nooftracks,
                                                  const QString &artist,
                                                  const QString &title,
                                                  const QString &date,
                                                  const QString &genre,
                                                  const QString &ext,
                                                  const QString &basepath,
                                                  bool overwrite_existing_files);

    bool check();

    QString target_dir;

    bool p_finished;
    bool finished_successful;
    void request_finish(bool successful);
    void execute_finish();

    int process_counter;
    bool timeout_done;
    int timeout_counter;

    /* PROCESS 1: RIPPING */
    QString ex_track_source_filename;
    QString ex_track_target_filename;
    int ex_track_index;
    int ex_track_count;
    QTimer *timer_extract;
    int current_sector;
    int overall_frames;

    /* PROCESS 2: ENCODING */
    QString en_track_filename;
    QString en_track_target_filename;
    int en_track_index;
    int en_track_count;
    QTimer *timer_encode;
    int current_encoder_percent;
    int last_checkpoint_encoder_percent;

    bool prepare_dir(QString &filename, const QString &targetDirIfRelative, const bool overwrite = false);
    bool mkdir(const QString &absoluteFilePath);
    qreal size_of_all_files(const QStringList &filenames) const;
};

}

#endif
