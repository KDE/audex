/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "audex.h"
#include "models/profilemodel.h"
#include "utils/schemeparser.h"

namespace Audex
{

AudexManager::AudexManager(QWidget *parent, ProfileModel *profile_model, CDDAModel *cdda_model)
    : QObject(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;
    this->cdda_model = cdda_model;

    p_profile_name = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_NAME_INDEX)).toString();
    p_suffix = profile_model->getSelectedEncoderSuffixFromCurrentIndex();
    p_single_file = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SF_INDEX)).toBool();

    encoder_wrapper = new EncoderWrapper(this,
                                         profile_model->getSelectedEncoderSchemeFromCurrentIndex(),
                                         profile_model->getSelectedEncoderNameAndVersion(),
                                         Preferences::deletePartialFiles());

    if (!encoder_wrapper) {
        qDebug() << "PANIC ERROR. Could not load object EncoderWrapper. Low mem?";
        return;
    }

    cdda_rip_thread = new CDDARipThread(cdda_model->blockDevice(), cdda_model->getToc());
    if (!cdda_rip_thread) {
        qDebug() << "PANIC ERROR. Could not load object CDDAExtractThread. Low mem?";
        return;
    }
    cdda_rip_thread->enableParanoiaMode(Preferences::fullParanoiaMode());
    cdda_rip_thread->enableSkipReadErrors(Preferences::skipReadErrors());
    cdda_rip_thread->setSampleShift(Preferences::sampleShift());

    jobs = new AudexJobs();
    connect(jobs, SIGNAL(newJobAvailable()), this, SLOT(start_encode()));

    wave_file_writer = new WaveFileWriter();

    last_measuring_point_sector = -1;
    timer_extract = new QTimer(this);
    connect(timer_extract, SIGNAL(timeout()), this, SLOT(calculate_speed_extract()));
    timer_extract->start(4000);

    last_measuring_point_encoder_percent = -1;
    timer_encode = new QTimer(this);
    connect(timer_encode, SIGNAL(timeout()), this, SLOT(calculate_speed_encode()));
    timer_encode->start(2000);

    connect(encoder_wrapper, SIGNAL(progress(int)), this, SLOT(progress_encode(int)));
    connect(encoder_wrapper, SIGNAL(finished()), this, SLOT(finish_encode()));
    connect(encoder_wrapper, SIGNAL(info(const QString &)), this, SLOT(slot_info(const QString &)));
    connect(encoder_wrapper, SIGNAL(warning(const QString &)), this, SLOT(slot_warning(const QString &)));
    connect(encoder_wrapper, SIGNAL(error(const QString &, const QString &)), this, SLOT(slot_error(const QString &, const QString &)));

    connect(cdda_rip_thread, SIGNAL(progress(int, int, int)), this, SLOT(progress_extract(int, int, int)));
    connect(cdda_rip_thread, SIGNAL(output(const QByteArray &)), this, SLOT(write_to_wave(const QByteArray &)));
    connect(cdda_rip_thread, SIGNAL(finished()), this, SLOT(finish_extract()));
    connect(cdda_rip_thread, SIGNAL(info(const QString &)), this, SLOT(slot_info(const QString &)));
    connect(cdda_rip_thread, SIGNAL(warning(const QString &)), this, SLOT(slot_warning(const QString &)));
    connect(cdda_rip_thread, SIGNAL(error(const QString &, const QString &)), this, SLOT(slot_error(const QString &, const QString &)));

    cdda_rip_thread->setTimestampsIntoLog(
        profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_LOG_WRITE_TIMESTAMPS_INDEX)).toBool());

    process_counter = 0;
    timeout_done = false;
    timeout_counter = 0;
    p_finished = false;
    p_finished_successful = false;

    en_track_index = 0;
    en_track_count = 0;

    ex_track_index = 0;
    ex_track_count = 0;

    current_sector = 0;
    current_encoder_percent = 0;

    overall_frames = 0;
}

AudexManager::~AudexManager()
{
    delete encoder_wrapper;
    delete cdda_rip_thread;
    delete wave_file_writer;
    delete jobs;
}

bool AudexManager::prepare()
{
    if (profile_model->currentProfileIndex() < 0) {
        slot_error(i18n("No profile selected. Operation abort."));
        return false;
    }

    qDebug() << "Using profile with index" << profile_model->currentProfileIndex();

    return true;
}

void AudexManager::start()
{
    Q_EMIT changedEncodeTrack(0, 0, "");
    Q_EMIT info(i18n("Start ripping and encoding with profile \"%1\"...", p_profile_name));
    if (check())
        start_extract();
    else
        request_finish(false);
}

void AudexManager::cancel()
{
    request_finish(false);
}

const QStringList &AudexManager::extractLog()
{
    return cdda_rip_thread->log();
}

const QStringList &AudexManager::encoderLog()
{
    return encoder_wrapper->log();
}

void AudexManager::start_extract()
{
    if (p_finished)
        return;

    if (p_single_file) {
        if (ex_track_count >= 1) {
            if (!jobs->jobInProgress() && !jobs->pendingJobs())
                request_finish(true);
            return;
        }

        ex_track_index++;

        QString artist = cdda_model->artist();
        QString title = cdda_model->title();
        QString year = cdda_model->year();
        QString genre = cdda_model->genre();
        QString suffix = p_suffix;
        bool fat32_compatible =
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool();
        bool replacespaceswithunderscores =
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool();
        QString basepath = Preferences::basePath();
        int cdnum;
        if (!cdda_model->isMultiCD())
            cdnum = 0;
        else
            cdnum = cdda_model->cdNum();
        int nooftracks = cdda_model->numOfAudioTracks();
        bool overwrite = Preferences::overwriteExistingFiles();

        QString targetFilename;
        if (!construct_target_filename_for_singlefile(targetFilename,
                                                      cdnum,
                                                      nooftracks,
                                                      artist,
                                                      title,
                                                      year,
                                                      genre,
                                                      suffix,
                                                      basepath,
                                                      overwrite,
                                                      fat32_compatible,
                                                      replacespaceswithunderscores)) {
            request_finish(false);
            return;
        }
        ex_track_target_filename = targetFilename;

        cdda_model->setCustomData("filename", targetFilename);

        // if empty (maybe because it already exists) skip
        if (!targetFilename.isEmpty()) {
            Q_EMIT changedExtractTrack(ex_track_index, 1, artist, title);

            QString sourceFilename = tmp_dir.path() + '/' + "tracks-all.wav";
            ex_track_source_filename = sourceFilename;
            wave_file_writer->open(sourceFilename);

            cdda_rip_thread->setTrackToRip(0);
            cdda_rip_thread->start();
            process_counter++;

            ex_track_count++;

        } else {
            if (!jobs->jobInProgress() && !jobs->pendingJobs())
                request_finish(true);
        }

    } else {
        if (ex_track_count >= cdda_model->numOfAudioTracksInSelection()) {
            if (!jobs->jobInProgress() && !jobs->pendingJobs())
                request_finish(true);
            return;
        }

        ex_track_index++;

        bool skip = !cdda_model->isTrackInSelection(ex_track_index);

        if (!cdda_model->isAudioTrack(ex_track_index))
            skip = true;

        if (!skip) {
            QString artist = cdda_model->artist();
            QString title = cdda_model->title();
            QString tartist = cdda_model->data(cdda_model->index(ex_track_index - 1, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString();
            QString ttitle = cdda_model->data(cdda_model->index(ex_track_index - 1, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString();
            QString year = cdda_model->year();
            QString genre = cdda_model->genre();
            QString suffix = p_suffix;
            QString basepath = Preferences::basePath();
            bool fat32_compatible =
                profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool();
            bool replacespaceswithunderscores =
                profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool();
            bool _2digitstracknum =
                profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX)).toBool();
            int cdnum;
            if (!cdda_model->isMultiCD())
                cdnum = 0;
            else
                cdnum = cdda_model->cdNum();
            int trackoffset = cdda_model->trackOffset();
            int nooftracks = cdda_model->numOfAudioTracks();
            bool overwrite = Preferences::overwriteExistingFiles();

            QString targetFilename;
            if (cdda_model->isVarious()) {
                if (!construct_target_filename(targetFilename,
                                               ex_track_index,
                                               cdnum,
                                               nooftracks,
                                               trackoffset,
                                               artist,
                                               title,
                                               tartist,
                                               ttitle,
                                               year,
                                               genre,
                                               QString(),
                                               suffix,
                                               basepath,
                                               fat32_compatible,
                                               replacespaceswithunderscores,
                                               _2digitstracknum,
                                               overwrite,
                                               (ex_track_index == 1))) {
                    request_finish(false);
                    return;
                }
            } else {
                if (!construct_target_filename(targetFilename,
                                               ex_track_index,
                                               cdnum,
                                               nooftracks,
                                               trackoffset,
                                               artist,
                                               title,
                                               artist,
                                               ttitle,
                                               year,
                                               genre,
                                               QString(),
                                               suffix,
                                               basepath,
                                               fat32_compatible,
                                               replacespaceswithunderscores,
                                               _2digitstracknum,
                                               overwrite,
                                               (ex_track_index == 1))) {
                    request_finish(false);
                    return;
                }
            }
            ex_track_target_filename = targetFilename;

            // if empty (maybe because it already exists) skip
            if (!targetFilename.isEmpty()) {
                Q_EMIT changedExtractTrack(ex_track_index, cdda_model->numOfAudioTracks(), tartist, ttitle);

                QString sourceFilename = tmp_dir.path() + '/' + QString("track-%1").arg(ex_track_index) + ".wav";
                ex_track_source_filename = sourceFilename;
                wave_file_writer->open(sourceFilename);

                cdda_rip_thread->setTrackToRip(ex_track_index);
                cdda_rip_thread->start();
                process_counter++;

                ex_track_count++;

            } else {
                en_track_count++;
                ex_track_count++; // important to check for finish
                cdda_rip_thread->skipTrack(ex_track_index);
                start_extract();
            }

        } else {
            start_extract();
        }
    }
}

void AudexManager::finish_extract()
{
    process_counter--;

    wave_file_writer->close();
    if (p_finished) {
        QFile file(ex_track_source_filename);
        file.remove();
        if (!process_counter)
            execute_finish();
        return;
    }
    jobs->addNewJob(ex_track_source_filename, ex_track_target_filename, ex_track_index);
    start_extract();
}

void AudexManager::start_encode()
{
    if (p_finished)
        return;

    if (p_single_file) {
        if (en_track_count >= 1) {
            request_finish(true);
            return;
        }

        if (encoder_wrapper->isProcessing())
            return;

        AudexJob *job = jobs->orderJob();
        if (!job)
            return;

        int cdnum = cdda_model->cdNum();
        int nooftracks = cdda_model->numOfAudioTracks();
        QString artist = cdda_model->artist();
        QString title = cdda_model->title();
        QString year = cdda_model->year();
        QString genre = cdda_model->genre();
        QString suffix = p_suffix;

        QString targetFilename = job->targetFilename();
        en_track_target_filename = targetFilename;

        Q_EMIT changedEncodeTrack(job->trackNo(), 1, targetFilename);
        en_track_count++;

        en_track_filename = job->sourceFilename();
        en_track_index = job->trackNo();
        if (!encoder_wrapper->encode(job->trackNo(),
                                     cdnum,
                                     0,
                                     nooftracks,
                                     artist,
                                     title,
                                     artist,
                                     title,
                                     genre,
                                     year,
                                     QString(),
                                     suffix,
                                     cdda_model->cover(),
                                     tmp_dir.path(),
                                     job->sourceFilename(),
                                     targetFilename)) {
            request_finish(false);
        }
        process_counter++;

    } else {
        if (en_track_count >= cdda_model->numOfAudioTracksInSelection()) {
            request_finish(true);
            return;
        }

        if (encoder_wrapper->isProcessing())
            return;

        AudexJob *job = jobs->orderJob();
        if (!job)
            return;

        int cdnum = cdda_model->cdNum();
        int trackoffset = cdda_model->trackOffset();
        int nooftracks = cdda_model->numOfAudioTracks();
        QString artist = cdda_model->artist();
        QString title = cdda_model->title();
        QString tartist = cdda_model->data(cdda_model->index(job->trackNo() - 1, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString();
        QString ttitle = cdda_model->data(cdda_model->index(job->trackNo() - 1, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString();
        QString year = cdda_model->year();
        QString genre = cdda_model->genre();
        QString suffix = p_suffix;

        QString targetFilename = job->targetFilename();
        en_track_target_filename = targetFilename;

        Q_EMIT changedEncodeTrack(job->trackNo(), cdda_model->numOfAudioTracks(), targetFilename);
        en_track_count++;

        en_track_filename = job->sourceFilename();
        en_track_index = job->trackNo();
        if (cdda_model->isVarious()) {
            if (!encoder_wrapper->encode(job->trackNo(),
                                         cdnum,
                                         trackoffset,
                                         nooftracks,
                                         artist,
                                         title,
                                         tartist,
                                         ttitle,
                                         genre,
                                         year,
                                         QString(),
                                         suffix,
                                         cdda_model->cover(),
                                         tmp_dir.path(),
                                         job->sourceFilename(),
                                         targetFilename)) {
                request_finish(false);
            }
        } else {
            if (!encoder_wrapper->encode(job->trackNo(),
                                         cdnum,
                                         trackoffset,
                                         nooftracks,
                                         artist,
                                         title,
                                         artist,
                                         ttitle,
                                         genre,
                                         year,
                                         QString(),
                                         suffix,
                                         cdda_model->cover(),
                                         tmp_dir.path(),
                                         job->sourceFilename(),
                                         targetFilename)) {
                request_finish(false);
            }
        }
        process_counter++;
    }
}

void AudexManager::finish_encode()
{
    process_counter--;
    jobs->reportJobFinished();

    cdda_model->setCustomDataPerTrack(en_track_index, "filename", en_track_target_filename);
    cdda_model->setCustomDataPerTrack(en_track_index, "ripped", true);

    QFile file(en_track_filename);
    file.remove();

    if (p_finished) {
        if (!process_counter)
            execute_finish();
        return;
    }
    Q_EMIT changedEncodeTrack(0, 0, "");
    progress_encode(0);
    start_encode();
}

void AudexManager::calculate_speed_extract()
{
    if ((last_measuring_point_sector > -1) && (cdda_rip_thread->isProcessing())) {
        double new_value = (double)(current_sector - last_measuring_point_sector) / (4.0f * (double)SECTORS_PER_SECOND);
        if (new_value < 0.0f)
            new_value = 0.0f;
        if ((new_value < 0.2f) && (!timeout_done)) {
            timeout_counter += 2;
            if (timeout_counter >= 300) {
                timeout_done = true;
                Q_EMIT timeout();
            }
        }
        Q_EMIT speedExtract(new_value);
    } else {
        Q_EMIT speedExtract(0.0f);
    }
    last_measuring_point_sector = current_sector;
}

void AudexManager::calculate_speed_encode()
{
    if ((last_measuring_point_encoder_percent > -1) && (encoder_wrapper->isProcessing()) && (current_encoder_percent > 0)) {
        int song_length = cdda_model->data(cdda_model->index(en_track_index - 1, CDDA_MODEL_COLUMN_LENGTH_INDEX), CDDA_MODEL_INTERNAL_ROLE).toInt();
        double new_value = (double)((double)song_length / 100.0f) * ((double)current_encoder_percent - (double)last_measuring_point_encoder_percent);
        if (new_value < 0.0f)
            new_value = 0.0f;
        Q_EMIT speedEncode(new_value);
    } else {
        Q_EMIT speedEncode(0.0f);
    }
    last_measuring_point_encoder_percent = current_encoder_percent;
}

void AudexManager::progress_extract(int percent_of_track, int sector, int overall_sectors_read)
{
    if (overall_frames == 0) {
        QSet<int> sel = cdda_model->selectedTracks();
        QSet<int>::ConstIterator it(sel.begin()), end(sel.end());
        for (; it != end; ++it) {
            if ((*it < 0) || (*it > cdda_model->numOfTracks()) || (!cdda_model->isAudioTrack((*it)))) {
                continue;
            }
            overall_frames += cdda_model->getToc().sectorCountTrack((*it));
        }
    }

    float fraction = 0.0f;
    if (overall_frames > 0)
        fraction = (float)overall_sectors_read / (float)overall_frames;

    Q_EMIT progressExtractTrack(percent_of_track);
    Q_EMIT progressExtractOverall((int)(fraction * 100.0f));

    current_sector = sector;
}

void AudexManager::progress_encode(int percent)
{
    Q_EMIT progressEncodeTrack(percent);
    if (percent > 0) {
        Q_EMIT progressEncodeOverall(((en_track_count > 0 ? ((en_track_count - 1) * 100.0f) : 0) + (percent * 1.0f))
                                     / (float)cdda_model->numOfAudioTracksInSelection());
    }
    current_encoder_percent = percent;
}

void AudexManager::write_to_wave(const QByteArray &data)
{
    wave_file_writer->write(data);
}

void AudexManager::slot_error(const QString &description, const QString &solution)
{
    Q_EMIT error(description, solution);
    request_finish(false);
}

void AudexManager::slot_warning(const QString &description)
{
    Q_EMIT warning(description);
}

void AudexManager::slot_info(const QString &description)
{
    Q_EMIT info(description);
}

void AudexManager::check_if_thread_still_running()
{
    if (cdda_rip_thread->isRunning()) {
        // this could happen if the thread is stuck in paranoia_read
        // because of an unreadable cd
        cdda_rip_thread->terminate();
        qDebug() << "Terminate extracting thread.";
    }
}

bool AudexManager::construct_target_filename(QString &targetFilename,
                                             int trackno,
                                             int cdno,
                                             int nooftracks,
                                             int gindex,
                                             const QString &artist,
                                             const QString &title,
                                             const QString &tartist,
                                             const QString &ttitle,
                                             const QString &year,
                                             const QString &genre,
                                             const QString &isrc,
                                             const QString &ext,
                                             const QString &basepath,
                                             bool fat32_compatible,
                                             bool replacespaceswithunderscores,
                                             bool _2digitstracknum,
                                             bool overwrite_existing_files,
                                             bool is_first_track)
{
    Q_UNUSED(is_first_track);

    SchemeParser schemeparser;
    QString basename = schemeparser.parsePerTrackFilenameScheme(
        profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SCHEME_INDEX)).toString(),
        trackno,
        cdno,
        gindex,
        nooftracks,
        defuse_for_filename(artist),
        defuse_for_filename(title),
        defuse_for_filename(tartist),
        defuse_for_filename(ttitle),
        defuse_for_filename(year),
        defuse_for_filename(genre),
        isrc,
        ext,
        fat32_compatible,
        replacespaceswithunderscores,
        _2digitstracknum);

    targetFilename = ((basepath.right(1) == "/") ? basepath : basepath + "/") + basename;

    int lastSlash = targetFilename.lastIndexOf("/", -1);
    if (lastSlash == -1) {
        Q_EMIT error(i18n("Can't find path \"%1\".", targetFilename), i18n("Please check your path (write access?)"));
        return false;
    }
    QString targetPath = targetFilename.mid(0, lastSlash);
    QString targetStrippedFilename = targetFilename.mid(lastSlash + 1);
    target_dir = targetPath;

    if (!p_mkdir(targetPath))
        return false;

    QStorageInfo diskfreespace(targetPath);
    quint64 free = diskfreespace.bytesAvailable() / 1024;
    if (free < 200 * 1024) {
        Q_EMIT warning(i18n("Free space on \"%1\" is less than 200 MiB.", targetPath));
    }

    auto *file = new QFile(targetFilename);
    if (file->exists()) {
        if (overwrite_existing_files) {
            Q_EMIT warning(i18n("Warning! File \"%1\" already exists. Overwriting.", targetStrippedFilename));
        } else {
            Q_EMIT warning(i18n("Warning! File \"%1\" already exists. Skipping.", targetStrippedFilename));

            cdda_model->setCustomDataPerTrack(trackno, "filename", targetFilename);
            cdda_model->setCustomDataPerTrack(trackno, "ripped", true);

            targetFilename.clear();
        }
    }
    delete file;

    return true;
}

bool AudexManager::construct_target_filename_for_singlefile(QString &targetFilename,
                                                            int cdno,
                                                            int nooftracks,
                                                            const QString &artist,
                                                            const QString &title,
                                                            const QString &date,
                                                            const QString &genre,
                                                            const QString &ext,
                                                            const QString &basepath,
                                                            bool fat32_compatible,
                                                            bool replacespaceswithunderscores,
                                                            bool overwrite_existing_files)
{
    SchemeParser schemeparser;
    targetFilename = ((basepath.right(1) == "/") ? basepath : basepath + "/")
        + schemeparser.parseFilenameScheme(
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SF_NAME_INDEX)).toString(),
            cdno,
            nooftracks,
            defuse_for_filename(artist),
            defuse_for_filename(title),
            defuse_for_filename(date),
            defuse_for_filename(genre),
            ext,
            fat32_compatible,
            replacespaceswithunderscores);

    int lastSlash = targetFilename.lastIndexOf("/", -1);
    if (lastSlash == -1) {
        Q_EMIT error(i18n("Can't find path \"%1\".", targetFilename), i18n("Please check your path (write access?)"));
        return false;
    }
    QString targetPath = targetFilename.mid(0, lastSlash);
    target_dir = targetPath;
    QString targetStrippedFilename = targetFilename.mid(lastSlash + 1);
    QDir *dir = new QDir(targetPath);
    if (!dir->exists()) {
        if (!dir->mkpath(targetPath)) {
            Q_EMIT error(i18n("Unable to create folder \"%1\".", targetPath), i18n("Please check your path (write access?)"));
            return false;
        } else {
            Q_EMIT info(i18n("Folder \"%1\" successfully created.", targetPath));
        }
    } else {
        Q_EMIT warning(i18n("Folder \"%1\" already exists.", targetPath));
    }
    delete dir;

    QStorageInfo diskfreespace(targetPath);
    quint64 free = diskfreespace.bytesAvailable() / 1024;
    if (free < 800 * 1024) {
        Q_EMIT warning(i18n("Free space on \"%1\" is less than 800 MiB.", targetPath));
    }

    auto *file = new QFile(targetFilename);
    if (file->exists()) {
        if (overwrite_existing_files) {
            Q_EMIT warning(i18n("Warning! File \"%1\" already exists. Overwriting.", targetStrippedFilename));
        } else {
            Q_EMIT warning(i18n("Warning! File \"%1\" already exists. Skipping.", targetStrippedFilename));

            cdda_model->setCustomData("filename", targetFilename);

            targetFilename.clear();
        }
    }
    delete file;

    return true;
}

bool AudexManager::check()
{
    if (!tmp_dir.isValid()) {
        slot_error(i18n("Temporary folder \"%1\" error (%2).", tmp_dir.path(), tmp_dir.errorString()), i18n("Please check."));
        return false;
    }

    QStorageInfo diskfreespace(tmp_dir.path());
    quint64 free = diskfreespace.bytesAvailable() / (1024 * 1024);

    if (free < 800) {
        slot_warning(i18n("Free space on temporary folder \"%1\" is less than 800 MiB.", tmp_dir.path()));
    } else if (free < 200) {
        slot_error(i18n("Temporary folder \"%1\" needs at least 200 MiB of free space.", tmp_dir.path()), i18n("Please free space or set another path."));
        return false;
    }

    return true;
}

void AudexManager::request_finish(bool successful)
{
    if (!p_finished) {
        p_finished = true;
        p_finished_successful = successful;
    } else {
        return;
    }

    if (process_counter > 0) {
        encoder_wrapper->cancel();
        cdda_rip_thread->cancel();
        QTimer::singleShot(2000, this, SLOT(check_if_thread_still_running()));

    } else {
        execute_finish();
    }
}

void AudexManager::execute_finish()
{
    if (Preferences::ejectCDTray()) {
        Q_EMIT info(i18n("Eject CD tray"));
        cdda_model->eject();
    }

    bool overwrite = Preferences::overwriteExistingFiles();

    QStringList target_filename_list;
    for (int i = 0; i < cdda_model->rowCount(); ++i) {
        if (!cdda_model->isAudioTrack(i + 1))
            continue;
        if (!cdda_model->isTrackInSelection(i + 1))
            continue;
        if (!cdda_model->getCustomDataPerTrack(i + 1, "ripped").toBool())
            continue;
        target_filename_list.append(cdda_model->getCustomDataPerTrack(i + 1, "filename").toString());
    }
    QString target_single_filename;
    if (p_single_file) {
        target_single_filename = cdda_model->customData("filename").toString();
    }

    QString cover_file;
    if ((p_finished_successful) && (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_INDEX)).toBool())) {
        // store the cover
        if (!cdda_model->isCoverEmpty()) {
            QImage image(cdda_model->cover());
            if (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_SCALE_INDEX)).toBool()) {
                QSize size = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_SIZE_INDEX)).toSize();
                image = image.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                qDebug() << QString("Cover scaled to %1x%2.").arg(size.width()).arg(size.height());
            }
            QString scheme = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_NAME_INDEX)).toString();
            QString format = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX)).toString();

            SchemeParser schemeparser;
            QString filename = schemeparser.parseFilenameScheme(scheme,
                                                                cdda_model->cdNum(),
                                                                cdda_model->numOfAudioTracks(),
                                                                defuse_for_filename(cdda_model->artist()),
                                                                defuse_for_filename(cdda_model->title()),
                                                                QString("%1").arg(cdda_model->year()),
                                                                defuse_for_filename(cdda_model->genre()),
                                                                format.toLower());

            if (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool())
                filename = SchemeParser::makeFAT32FilenameCompatible(filename);

            if (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool())
                filename = SchemeParser::replaceSpacesWithUnderscores(filename);

            if (p_prepare_dir(filename, target_dir, overwrite)) {
                if (image.save(filename, format.toLatin1().data())) {
                    Q_EMIT info(i18n("Cover \"%1\" successfully saved.", QFileInfo(filename).fileName()));
                    cover_file = filename;
                } else {
                    Q_EMIT error(i18n("Unable to save cover \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));
                }
            }
        }
    }

    QString playlist_file;
    if ((p_finished_successful) && (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_PL_INDEX)).toBool())
        && (target_filename_list.count() > 0) && (!p_single_file)) {
        // create the playlist
        Playlist playlist;

        QString scheme = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_PL_NAME_INDEX)).toString();
        QString format = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_PL_FORMAT_INDEX)).toString();
        bool is_absFilePath =
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX)).toBool();
        bool is_utf8 = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_PL_UTF8_INDEX)).toBool();

        SchemeParser schemeparser;
        QString filename = schemeparser.parseFilenameScheme(scheme,
                                                            cdda_model->cdNum(),
                                                            cdda_model->numOfAudioTracks(),
                                                            defuse_for_filename(cdda_model->artist()),
                                                            defuse_for_filename(cdda_model->title()),
                                                            QString("%1").arg(cdda_model->year()),
                                                            defuse_for_filename(cdda_model->genre()),
                                                            format.toLower());

        if (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool())
            filename = SchemeParser::makeFAT32FilenameCompatible(filename);

        if (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool())
            filename = SchemeParser::replaceSpacesWithUnderscores(filename);

        if (p_prepare_dir(filename, target_dir, (overwrite && !cdda_model->isMultiCD() && (cdda_model->cdNum() < 1)))) {
            QFile file(filename);

            if (file.exists() && cdda_model->isMultiCD() && (cdda_model->cdNum() > 0)) {
                if (file.open(QFile::ReadOnly)) {
                    QByteArray ba = file.readAll();
                    playlist.addPlaylist(ba);
                    file.close();
                }
            }

            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                for (int i = 0; i < cdda_model->rowCount(); ++i) {
                    if (!cdda_model->isAudioTrack(i + 1))
                        continue;
                    if (!cdda_model->isTrackInSelection(i + 1))
                        continue;
                    if (!cdda_model->getCustomDataPerTrack(i + 1, "ripped").toBool())
                        continue;
                    PlaylistItem item;
                    item.setFilename(cdda_model->getCustomDataPerTrack(i + 1, "filename").toString());
                    item.setArtist(cdda_model->data(cdda_model->index(i, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString());
                    item.setTitle(cdda_model->data(cdda_model->index(i, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString());
                    item.setLength(cdda_model->lengthOfTrack(i + 1));
                    playlist.appendItem(item);
                }

                QString relFilePath;
                if (!is_absFilePath) {
                    relFilePath = QFileInfo(filename).absoluteDir().absolutePath();
                }

                if (format == "M3U") {
                    file.write(playlist.toM3U(relFilePath, is_utf8));
                } else if (format == "PLS") {
                    file.write(playlist.toPLS(relFilePath, is_utf8));
                } else if (format == "XSPF") {
                    file.write(playlist.toXSPF());
                }
                file.close();
                Q_EMIT info(i18n("Playlist \"%1\" successfully created.", QFileInfo(filename).fileName()));
                playlist_file = filename;

            } else {
                Q_EMIT error(i18n("Unable to save playlist \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));
            }
        }
    }

    QString info_file;
    if ((p_finished_successful) && (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_INF_INDEX)).toBool())) {
        SchemeParser schemeparser;
        QString filename = schemeparser.parseFilenameScheme(
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_INF_NAME_INDEX)).toString(),
            cdda_model->cdNum(),
            cdda_model->numOfAudioTracks(),
            defuse_for_filename(cdda_model->artist()),
            defuse_for_filename(cdda_model->title()),
            QString("%1").arg(cdda_model->year()),
            defuse_for_filename(cdda_model->genre()),
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_INF_SUFFIX_INDEX)).toString());

        if (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool())
            filename = SchemeParser::makeFAT32FilenameCompatible(filename);

        if (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool())
            filename = SchemeParser::replaceSpacesWithUnderscores(filename);

        if (p_prepare_dir(filename, target_dir, overwrite)) {
            QFile file(filename);
            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream out(&file);
                QStringList text =
                    profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_INF_TEXT_INDEX)).toStringList();
                schemeparser.parseInfoTextScheme(text,
                                                 cdda_model->artist(),
                                                 cdda_model->title(),
                                                 QString("%1").arg(cdda_model->year()),
                                                 cdda_model->genre(),
                                                 QString(),
                                                 DiscIDCalculator::CDDBId(cdda_model->discSignature()),
                                                 p_size_of_all_files(target_filename_list),
                                                 cdda_model->lengthOfAudioTracksInSelection(),
                                                 cdda_model->numOfAudioTracksInSelection());
                out << text.join("\n");
                file.close();
                Q_EMIT info(i18n("Info file \"%1\" successfully created.", QFileInfo(filename).fileName()));
                info_file = filename;
            } else {
                Q_EMIT error(i18n("Unable to save info file \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));
            }
        }
    }

    QString hash_list;
    if ((p_finished_successful) && (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_HL_INDEX)).toBool())
        && (target_filename_list.count() > 0)) {
        QString scheme = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_HL_NAME_INDEX)).toString();
        QString format = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_HL_FORMAT_INDEX)).toString();

        SchemeParser schemeparser;
        QString filename = schemeparser.parseFilenameScheme(
            scheme,
            cdda_model->cdNum(),
            cdda_model->numOfAudioTracks(),
            defuse_for_filename(cdda_model->artist()),
            defuse_for_filename(cdda_model->title()),
            QString("%1").arg(cdda_model->year()),
            defuse_for_filename(cdda_model->genre()),
            format.toLower(),
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool(),
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool());

        if (p_prepare_dir(filename, target_dir, (overwrite && !cdda_model->isMultiCD() && (cdda_model->cdNum() < 1)))) {
            QFile file(filename);
            bool fexists = file.exists() && cdda_model->isMultiCD() && (cdda_model->cdNum() > 0);
            bool success;
            if (fexists)
                success = file.open(QFile::WriteOnly | QFile::Append);
            else
                success = file.open(QFile::WriteOnly | QFile::Truncate);
            if (success) {
                QTextStream out(&file);
                if (fexists)
                    out << "\n";
                Hashlist hashlist;
                if (format == "SFV") {
                    out << hashlist.getSFV(target_filename_list).join("\n");
                } else if (format == "MD5") {
                    out << hashlist.getMD5(target_filename_list).join("\n");
                } else if (format == "SHA-256") {
                    out << hashlist.getSHA256(target_filename_list).join("\n");
                }
                file.close();
                Q_EMIT info(i18n("Hashlist \"%1\" successfully created.", QFileInfo(filename).fileName()));
                hash_list = filename;
            } else {
                Q_EMIT error(i18n("Unable to save hashlist \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));
            }
        }
    }

    QString cue_sheet;
    if ((p_finished_successful) && (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_CUE_INDEX)).toBool())
        && (((target_filename_list.count() > 0) && !p_single_file) || p_single_file)) {
        QString scheme = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_CUE_NAME_INDEX)).toString();
        // bool writeMCNAndISRC =
        //     profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_CUE_ADD_MCN_AND_ISRC_INDEX)).toBool();

        SchemeParser schemeparser;
        QString filename = schemeparser.parseFilenameScheme(
            scheme,
            cdda_model->cdNum(),
            cdda_model->numOfAudioTracks(),
            defuse_for_filename(cdda_model->artist()),
            defuse_for_filename(cdda_model->title()),
            QString("%1").arg(cdda_model->year()),
            defuse_for_filename(cdda_model->genre()),
            "cue",
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool(),
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool());

        if (p_prepare_dir(filename, target_dir, overwrite)) {
            QFile file(filename);
            bool success = file.open(QFile::WriteOnly | QFile::Truncate);
            if (success) {
                QTextStream out(&file);
                CueSheetWriter cuesheetwriter(cdda_model);
                if (p_single_file) {
                    out << cuesheetwriter.cueSheet(target_single_filename, Preferences::sampleShift() / SECTOR_SIZE_SAMPLES).join("\n");
                } else {
                    out << cuesheetwriter.cueSheet(target_filename_list, Preferences::sampleShift() / SECTOR_SIZE_SAMPLES).join("\n");
                }
                file.close();
                Q_EMIT info(i18n("Cue sheet \"%1\" successfully created.", QFileInfo(filename).fileName()));
                cue_sheet = filename;
            } else {
                Q_EMIT error(i18n("Unable to save cue sheet \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));
            }
        }
    }

    QString log_file;
    if ((p_finished_successful) && (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_LOG_INDEX)).toBool())) {
        SchemeParser schemeparser;
        QString filename = schemeparser.parseFilenameScheme(
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_LOG_NAME_INDEX)).toString(),
            cdda_model->cdNum(),
            cdda_model->numOfAudioTracks(),
            defuse_for_filename(cdda_model->artist()),
            defuse_for_filename(cdda_model->title()),
            QString("%1").arg(cdda_model->year()),
            defuse_for_filename(cdda_model->genre()),
            "log",
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool(),
            profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool());

        if (p_prepare_dir(filename, target_dir, overwrite)) {
            QFile file(filename);
            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream out(&file);
                out << cdda_rip_thread->log().join("\n");
                file.close();
                Q_EMIT info(i18n("Log file \"%1\" successfully stored.", QFileInfo(filename).fileName()));
                info_file = filename;
            } else {
                Q_EMIT error(i18n("Unable to save log file \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));
            }
        }
    }

    if ((p_finished_successful) && (Preferences::upload()) && (target_filename_list.count() > 0)) {
        QString targetpath = QFileInfo(target_filename_list.at(0)).absolutePath().mid(Preferences::basePath().length());

        QStringList files_to_transfer = target_filename_list;
        if (!cover_file.isEmpty())
            files_to_transfer << cover_file;
        if (!playlist_file.isEmpty())
            files_to_transfer << playlist_file;
        if (!info_file.isEmpty())
            files_to_transfer << info_file;
        if (!hash_list.isEmpty())
            files_to_transfer << hash_list;
        if (!cue_sheet.isEmpty())
            files_to_transfer << cue_sheet;

        Upload upload(Preferences::url(), this);
        connect(&upload, SIGNAL(info(const QString &)), this, SLOT(slot_info(const QString &)));
        connect(&upload, SIGNAL(error(const QString &, const QString &)), this, SLOT(slot_error(const QString &, const QString &)));
        upload.upload(targetpath, files_to_transfer);
    }

    // flush temporary path
    tmp_dir.remove();

    Q_EMIT finished(p_finished_successful);
}

bool AudexManager::p_prepare_dir(QString &filename, const QString &targetDirIfRelative, const bool overwrite)
{
    QString result;

    QFileInfo fileinfo(filename);
    if (fileinfo.isAbsolute()) {
        if (!p_mkdir(fileinfo.dir().absolutePath())) {
            return false;
        } else {
            result = filename;
        }
    } else {
        if (!targetDirIfRelative.isEmpty()) {
            QDir dir(targetDirIfRelative);
            if (!dir.isReadable()) {
                Q_EMIT error(i18n("Unable to open folder \"%1\".", targetDirIfRelative), i18n("Please check your path and permissions"));
                return false;
            }
            result = targetDirIfRelative + '/' + filename;
        } else {
            result = filename;
        }
    }

    if (!overwrite) {
        QFileInfo info(result);
        if (info.exists()) {
            Q_EMIT warning(i18n("Warning! File \"%1\" already exists. Skipping.", info.fileName()));
            return false;
        }
    }

    filename = result;

    return true;
}

bool AudexManager::p_mkdir(const QString &absoluteFilePath)
{
    QDir dir(absoluteFilePath);
    if (dir.exists()) {
        if (!dir.isReadable()) {
            Q_EMIT error(i18n("Unable to open folder \"%1\".", absoluteFilePath), i18n("Please check your path and permissions"));
            return false;
        }
    } else {
        if (!dir.mkpath(absoluteFilePath)) {
            Q_EMIT error(i18n("Unable to create folder \"%1\".", absoluteFilePath), i18n("Please check your path (write access?)"));
            return false;
        } else {
            Q_EMIT info(i18n("Folder \"%1\" successfully created.", absoluteFilePath));
        }
    }

    return true;
}

qreal AudexManager::p_size_of_all_files(const QStringList &filenames) const
{
    qreal size = .0f;
    for (int i = 0; i < filenames.count(); ++i) {
        QFileInfo info(filenames.at(i));
        size += info.size();
    }
    return size;
}

}
