/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AUDEX_HEADER
#define AUDEX_HEADER

#include <QDir>
#include <QQueue>
#include <QStorageInfo>
#include <QString>
#include <QTemporaryDir>
#include <QTimer>

#include <KLocalizedString>

#include "models/cddamodel.h"
#include "models/profilemodel.h"
#include "utils/cddaextractthread.h"
#include "utils/cuesheetwriter.h"
#include "utils/discidcalculator.h"
#include "utils/encoderwrapper.h"
#include "utils/hashlist.h"
#include "utils/parameters.h"
#include "utils/schemeparser.h"
#include "utils/playlist.h"
#include "utils/upload.h"
#include "utils/wavefilewriter.h"

#include "preferences.h"

class AudexJob : public QObject
{
    Q_OBJECT
public:
    explicit AudexJob(QObject *parent = nullptr)
        : QObject(parent)
    {
        source_filename = "";
        target_filename = "";
        _trackno = 0;
    }
    ~AudexJob() override
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
    void setTrackNo(const int t)
    {
        _trackno = t;
    }
    int trackNo() const
    {
        return _trackno;
    }

private:
    QString source_filename;
    QString target_filename;
    int _trackno;
};

class AudexJobs : public QObject
{
    Q_OBJECT
public:
    explicit AudexJobs(QObject *parent = nullptr)
        : QObject(parent)
    {
        job_in_progress = false;
    }
    ~AudexJobs() override
    {
        for (int i = 0; i < cache.count(); i++) {
            // make really sure all files are away
            QFile file(cache.at(i)->sourceFilename());
            if (file.exists())
                file.remove();
            delete cache.at(i);
        }
    }
    AudexJob *orderJob()
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
    void addNewJob(const QString &sourceFilename, const QString &targetFilename, const int trackno)
    {
        auto *j = new AudexJob();
        j->setSourceFilename(sourceFilename);
        j->setTargetFilename(targetFilename);
        j->setTrackNo(trackno);
        job_queue.enqueue(j);
        cache.append(j);
        Q_EMIT newJobAvailable();
    }
Q_SIGNALS:
    void newJobAvailable();

private:
    QQueue<AudexJob *> job_queue;
    QList<AudexJob *> cache;
    bool job_in_progress;
};

class Audex : public QObject
{
    Q_OBJECT

public:
    Audex(QWidget *parent, ProfileModel *profile_model, CDDAModel *cdda_model);
    ~Audex() override;

    bool prepare();

public Q_SLOTS:
    void start();
    void cancel();

    const QStringList &extractProtocol();
    const QStringList &encoderProtocol();

private Q_SLOTS:
    void start_extract();
    void finish_extract();
    void start_encode();
    void finish_encode();

    void calculate_speed_extract();
    void calculate_speed_encode();

    void progress_extract(int percent_of_track, int sector, int overall_sectors_read);
    void progress_encode(int percent);

    void write_to_wave(const QByteArray &data);

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

    void speedExtract(double times);
    void speedEncode(double times);

    void finished(bool successful);

    void timeout();

    void error(const QString &message, const QString &details = QString());
    void warning(const QString &message);
    void info(const QString &message);

private:
    QWidget *parent;
    ProfileModel *profile_model;
    CDDAModel *cdda_model;
    EncoderWrapper *encoder_wrapper;
    CDDAExtractThread *cdda_extract_thread;
    AudexJobs *jobs;
    WaveFileWriter *wave_file_writer;
    QTemporaryDir tmp_dir;

    QString p_profile_name;
    QString p_suffix;
    bool p_single_file;

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

    bool _finished;
    bool _finished_successful;
    void request_finish(bool successful);
    void execute_finish();

    int process_counter;
    bool timeout_done;
    int timeout_counter;

    /*PROCESS 1: EXTRACTING*/
    QString ex_track_source_filename;
    QString ex_track_target_filename;
    int ex_track_index;
    int ex_track_count;
    QTimer *timer_extract;
    int current_sector;
    int last_measuring_point_sector;
    int overall_frames;

    /*PROCESS 2: ENCODING*/
    QString en_track_filename;
    QString en_track_target_filename;
    int en_track_index;
    int en_track_count;
    QTimer *timer_encode;
    int current_encoder_percent;
    int last_measuring_point_encoder_percent;

    bool p_prepare_dir(QString &filename, const QString &targetDirIfRelative, const bool overwrite = false);
    bool p_mkdir(const QString &absoluteFilePath);
    qreal p_size_of_all_files(const QStringList &filenames) const;
};

#endif
