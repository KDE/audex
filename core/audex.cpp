/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2014 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "audex.h"

/* The heart of audex */

Audex::Audex(QWidget* parent, ProfileModel *profile_model, CDDAModel *cdda_model) : QObject(parent) {

  Q_UNUSED(parent);

  this->profile_model = profile_model;
  this->cdda_model = cdda_model;

  p_profile_name = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_NAME_INDEX)).toString();
  p_suffix = profile_model->getSelectedEncoderSuffixFromCurrentIndex();
  p_single_file = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SF_INDEX)).toBool();

  encoder_wrapper = new EncoderWrapper(this,
                        profile_model->getSelectedEncoderPatternFromCurrentIndex(),
                        profile_model->getSelectedEncoderNameAndVersion(),
                        Preferences::deletePartialFiles());

  if (!encoder_wrapper) {
    kDebug() << "PANIC ERROR. Could not load object EncoderWrapper. Low mem?";
    return;
  }

  cdda_extract_thread = new CDDAExtractThread(this, cdda_model->device());
  if (!cdda_extract_thread) {
    kDebug() << "PANIC ERROR. Could not load object CDDAExtractThread. Low mem?";
    return;
  }
  cdda_extract_thread->setSampleOffset(Preferences::sampleOffset());

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
  connect(encoder_wrapper, SIGNAL(info(const QString&)), this, SLOT(slot_info(const QString&)));
  connect(encoder_wrapper, SIGNAL(warning(const QString&)), this, SLOT(slot_warning(const QString&)));
  connect(encoder_wrapper, SIGNAL(error(const QString&, const QString&)), this, SLOT(slot_error(const QString&, const QString&)));

  connect(cdda_extract_thread, SIGNAL(progress(int, int, int)), this, SLOT(progress_extract(int, int, int)));
  connect(cdda_extract_thread, SIGNAL(output(const QByteArray&)), this, SLOT(write_to_wave(const QByteArray&)));
  connect(cdda_extract_thread, SIGNAL(finished()), this, SLOT(finish_extract()));
  connect(cdda_extract_thread, SIGNAL(terminated()), this, SLOT(finish_extract()));
  connect(cdda_extract_thread, SIGNAL(info(const QString&)), this, SLOT(slot_info(const QString&)));
  connect(cdda_extract_thread, SIGNAL(warning(const QString&)), this, SLOT(slot_warning(const QString&)));
  connect(cdda_extract_thread, SIGNAL(error(const QString&, const QString&)), this, SLOT(slot_error(const QString&, const QString&)));

  process_counter = 0;
  timeout_done = FALSE;
  timeout_counter = 0;
  _finished = FALSE;
  _finished_successful = FALSE;

  en_track_index = 0;
  en_track_count = 0;

  ex_track_index = 0;
  ex_track_count = 0;

  current_sector = 0;
  current_encoder_percent = 0;

  overall_frames = 0;

}

Audex::~Audex() {

  delete encoder_wrapper;
  delete cdda_extract_thread;
  delete wave_file_writer;
  delete jobs;
  delete tmp_dir;

}

bool Audex::prepare() {

  if (profile_model->currentProfileIndex() < 0) {
    slot_error(i18n("No profile selected. Operation abort."));
    return FALSE;
  }

  kDebug() << "Using profile with index" << profile_model->currentProfileIndex();

  tmp_dir = new TmpDir("audex", "work");
  tmp_path = tmp_dir->tmpPath();
  if (tmp_dir->error()) return FALSE;

  return TRUE;

}

void Audex::start() {

  emit changedEncodeTrack(0, 0, "");
  emit info(i18n("Start ripping and encoding with profile \"%1\"...", p_profile_name));
  if (check()) start_extract(); else request_finish(FALSE);

}

void Audex::cancel() {

  request_finish(FALSE);

}

const QStringList& Audex::extractProtocol() {
  return cdda_extract_thread->protocol();
}

const QStringList& Audex::encoderProtocol() {
  return encoder_wrapper->protocol();
}

void Audex::start_extract() {

  if (_finished) return;

  if (p_single_file) {

    if (ex_track_count >= 1) {
      if (!jobs->jobInProgress() && !jobs->pendingJobs()) request_finish(TRUE);
      return;
    }

    ex_track_index++;

    QString artist = cdda_model->artist();
    QString title = cdda_model->title();
    QString year = cdda_model->year();
    QString genre = cdda_model->genre();
    QString suffix = p_suffix;
    QString basepath = Preferences::basePath();
    int cdnum;
    if (!cdda_model->isMultiCD()) cdnum = 0; else cdnum = cdda_model->cdNum();
    int nooftracks = cdda_model->numOfAudioTracks();
    bool overwrite = Preferences::overwriteExistingFiles();

    QString targetFilename;
    if (!construct_target_filename_for_singlefile(targetFilename, cdnum, nooftracks, artist, title, year, genre, suffix, basepath, overwrite)) {
      request_finish(FALSE);
      return;
    }
    ex_track_target_filename = targetFilename;

    cdda_model->setCustomData("filename", targetFilename);

    //if empty (maybe because it already exists) skip
    if (!targetFilename.isEmpty()) {

      emit changedExtractTrack(ex_track_index,
        1,
        artist,
        title);

      QString sourceFilename = tmp_path+QString("%1").arg(DiscIDCalculator::FreeDBId(cdda_model->discSignature()))+".wav";
      ex_track_source_filename = sourceFilename;
      wave_file_writer->open(sourceFilename);

      if (Preferences::paranoiaMode()) {
        cdda_extract_thread->setParanoiaMode(3);
      } else {
        cdda_extract_thread->setParanoiaMode(0);
      }
      cdda_extract_thread->setNeverSkip(Preferences::neverSkip());
      cdda_extract_thread->setTrackToRip(0);
      cdda_extract_thread->start(); process_counter++;

      ex_track_count++;

    } else {

      if (!jobs->jobInProgress() && !jobs->pendingJobs()) request_finish(TRUE);

    }

  } else {

    if (ex_track_count >= cdda_model->numOfAudioTracksInSelection()) {
      if (!jobs->jobInProgress() && !jobs->pendingJobs()) request_finish(TRUE);
      return;
    }

    ex_track_index++;

    bool skip = !cdda_model->isTrackInSelection(ex_track_index);

    if (!cdda_model->isAudioTrack(ex_track_index)) skip = TRUE;

    if (!skip) {

      QString artist = cdda_model->artist();
      QString title = cdda_model->title();
      QString tartist = cdda_model->data(cdda_model->index(ex_track_index-1, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString();
      QString ttitle = cdda_model->data(cdda_model->index(ex_track_index-1, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString();
      QString year = cdda_model->year();
      QString genre = cdda_model->genre();
      QString suffix = p_suffix;
      QString basepath = Preferences::basePath();
      bool fat32_compatible = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool();
      bool replacespaceswithunderscores = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool();
      bool _2digitstracknum = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX)).toBool();
      int cdnum;
      if (!cdda_model->isMultiCD()) cdnum = 0; else cdnum = cdda_model->cdNum();
      int trackoffset = cdda_model->trackOffset();
      int nooftracks = cdda_model->numOfAudioTracks();
      bool overwrite = Preferences::overwriteExistingFiles();

      QString targetFilename;
      if (cdda_model->isVarious()) {
        if (!construct_target_filename(targetFilename, ex_track_index, cdnum, nooftracks, trackoffset,
            artist, title, tartist, ttitle, year, genre, suffix, basepath, fat32_compatible, replacespaceswithunderscores, _2digitstracknum, overwrite, (ex_track_index==1))) {
          request_finish(FALSE);
          return;
        }
      } else {
        if (!construct_target_filename(targetFilename, ex_track_index, cdnum, nooftracks, trackoffset,
            artist, title, artist, ttitle, year, genre, suffix, basepath, fat32_compatible, replacespaceswithunderscores, _2digitstracknum, overwrite, (ex_track_index==1))) {
          request_finish(FALSE);
          return;
        }
      }
      ex_track_target_filename = targetFilename;

      //if empty (maybe because it already exists) skip
      if (!targetFilename.isEmpty()) {

        emit changedExtractTrack(ex_track_index,
          cdda_model->numOfAudioTracks(),
          tartist,
          ttitle);

        QString sourceFilename = tmp_path+QString("%1").arg(DiscIDCalculator::FreeDBId(cdda_model->discSignature()))+"."+QString("%1").arg(ex_track_index)+".wav";
        ex_track_source_filename = sourceFilename;
        wave_file_writer->open(sourceFilename);

        if (Preferences::paranoiaMode()) {
          cdda_extract_thread->setParanoiaMode(3);
        } else {
          cdda_extract_thread->setParanoiaMode(0);
        }
        cdda_extract_thread->setNeverSkip(Preferences::neverSkip());
        cdda_extract_thread->setTrackToRip(ex_track_index);
        cdda_extract_thread->start(); process_counter++;

        ex_track_count++;

      } else {

        en_track_count++;
        ex_track_count++; //important to check for finish
        cdda_extract_thread->skipTrack(ex_track_index);
        start_extract();

      }

    } else {

      start_extract();

    }

  }

}

void Audex::finish_extract() {

  process_counter--;

  wave_file_writer->close();
  if (_finished) {
    QFile file(ex_track_source_filename);
    file.remove();
    if (!process_counter) execute_finish();
    return;
  }
  jobs->addNewJob(ex_track_source_filename, ex_track_target_filename, ex_track_index);
  start_extract();

}

void Audex::start_encode() {

  if (_finished) return;

  if (p_single_file) {

    if (en_track_count >= 1) {
      request_finish(TRUE);
      return;
    }

    if (encoder_wrapper->isProcessing()) return;

    AudexJob* job = jobs->orderJob();
    if (!job) return;

    int cdnum = cdda_model->cdNum();
    int nooftracks = cdda_model->numOfAudioTracks();
    QString artist = cdda_model->artist();
    QString title = cdda_model->title();
    QString year = cdda_model->year();
    QString genre = cdda_model->genre();
    QString suffix = p_suffix;
    CachedImage *cover = cdda_model->cover();

    QString targetFilename = job->targetFilename();
    en_track_target_filename = targetFilename;

    emit changedEncodeTrack(job->trackNo(), 1, targetFilename);
    en_track_count++;

    en_track_filename = job->sourceFilename();
    en_track_index = job->trackNo();
    if (!encoder_wrapper->encode(job->trackNo(), cdnum, 0, nooftracks,
          artist, title, artist, title, genre, year, suffix, cover, FALSE, tmp_path,
          job->sourceFilename(), targetFilename)) {
      request_finish(FALSE);
    }
    process_counter++;

  } else {

    if (en_track_count >= cdda_model->numOfAudioTracksInSelection()) {
      request_finish(TRUE);
      return;
    }

    if (encoder_wrapper->isProcessing()) return;

    AudexJob* job = jobs->orderJob();
    if (!job) return;

    int cdnum = cdda_model->cdNum();
    int trackoffset = cdda_model->trackOffset();
    int nooftracks = cdda_model->numOfAudioTracks();
    QString artist = cdda_model->artist();
    QString title = cdda_model->title();
    QString tartist = cdda_model->data(cdda_model->index(job->trackNo()-1, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString();
    QString ttitle = cdda_model->data(cdda_model->index(job->trackNo()-1, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString();
    QString year = cdda_model->year();
    QString genre = cdda_model->genre();
    QString suffix = p_suffix;
    CachedImage *cover = cdda_model->cover();
    bool fat32_compatible = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool();

    QString targetFilename = job->targetFilename();
    en_track_target_filename = targetFilename;

    emit changedEncodeTrack(job->trackNo(), cdda_model->numOfAudioTracks(), targetFilename);
    en_track_count++;

    en_track_filename = job->sourceFilename();
    en_track_index = job->trackNo();
    if (cdda_model->isVarious()) {
      if (!encoder_wrapper->encode(job->trackNo(), cdnum, trackoffset, nooftracks,
            artist, title, tartist, ttitle, genre, year, suffix, cover, fat32_compatible, tmp_path,
            job->sourceFilename(), targetFilename)) {
        request_finish(FALSE);
      }
    } else {
      if (!encoder_wrapper->encode(job->trackNo(), cdnum, trackoffset, nooftracks,
            artist, title, artist, ttitle, genre, year, suffix, cover, fat32_compatible, tmp_path,
            job->sourceFilename(), targetFilename)) {
        request_finish(FALSE);
      }
    }
    process_counter++;

  }

}

void Audex::finish_encode() {

  process_counter--;
  jobs->reportJobFinished();

  cdda_model->setCustomDataPerTrack(en_track_index, "filename", en_track_target_filename);
  cdda_model->setCustomDataPerTrack(en_track_index, "ripped", TRUE);

  QFile file(en_track_filename);
  file.remove();

  if (_finished) {
    if (!process_counter) execute_finish();
    return;
  }
  emit changedEncodeTrack(0, 0, "");
  progress_encode(0);
  start_encode();

}

void Audex::calculate_speed_extract() {
  if ((last_measuring_point_sector > -1) && (cdda_extract_thread->isProcessing())) {
    double new_value = (double)(current_sector-last_measuring_point_sector)/(2.0f*75.0f);
    if (new_value < 0.0f) new_value = 0.0f;
    if ((new_value < 0.2f) && (!timeout_done)) {
      timeout_counter += 2;
      if (timeout_counter >= 300) { timeout_done = TRUE; emit timeout(); }
    }
    emit speedExtract(new_value);
  } else {
    emit speedExtract(0.0f);
  }
  last_measuring_point_sector = current_sector;
}

void Audex::calculate_speed_encode() {
  if ((last_measuring_point_encoder_percent > -1) && (encoder_wrapper->isProcessing()) && (current_encoder_percent > 0)) {
    int song_length = cdda_model->data(cdda_model->index(en_track_index-1, CDDA_MODEL_COLUMN_LENGTH_INDEX), CDDA_MODEL_INTERNAL_ROLE).toInt();
    double new_value = (double)((double)song_length/100.0f)*((double)current_encoder_percent-(double)last_measuring_point_encoder_percent);
    if (new_value < 0.0f) new_value = 0.0f;
    emit speedEncode(new_value);
  } else {
    emit speedEncode(0.0f);
  }
  last_measuring_point_encoder_percent = current_encoder_percent;
}

void Audex::progress_extract(int percent_of_track, int sector, int overall_sectors_read) {

  if (overall_frames==0) {
    QSet<int> sel = cdda_model->selectedTracks();
    QSet<int>::ConstIterator it(sel.begin()), end(sel.end());
    for (; it!=end; ++it) {
      if ((*it < 0) || (*it > cdda_extract_thread->cddaParanoia()->numOfTracks()) ||
           (!cdda_extract_thread->cddaParanoia()->isAudioTrack((*it)))) {
        continue;
      }
      overall_frames += cdda_extract_thread->cddaParanoia()->numOfFramesOfTrack((*it));
    }
  }

  float fraction = 0.0f;
  if (overall_frames > 0) fraction = (float)overall_sectors_read / (float)overall_frames;

  emit progressExtractTrack(percent_of_track);
  emit progressExtractOverall((int)(fraction*100.0f));

  current_sector = sector;

}

void Audex::progress_encode(int percent) {
  emit progressEncodeTrack(percent);
  if (percent > 0) {
    emit progressEncodeOverall( ((en_track_count>0 ? ((en_track_count-1)*100.0f) : 0) + (percent*1.0f)) /
                                 (float)cdda_model->numOfAudioTracksInSelection() );
  }
  current_encoder_percent = percent;
}

void Audex::write_to_wave(const QByteArray& data) {
  wave_file_writer->write(data);
}

void Audex::slot_error(const QString& description, const QString& solution) {
  emit error(description, solution);
  request_finish(FALSE);
}

void Audex::slot_warning(const QString& description) {
  emit warning(description);
}

void Audex::slot_info(const QString& description) {
  emit info(description);
}

void Audex::check_if_thread_still_running() {
  if (cdda_extract_thread->isRunning()) {
    //this could happen if the thread is stuck in paranoia_read
    //because of an unreadable cd
    cdda_extract_thread->terminate();
    kDebug() << "Terminate extracting thread.";
  }
}

bool Audex::construct_target_filename(QString& targetFilename,
        int trackno, int cdno, int nooftracks, int gindex,
        const QString& artist, const QString& title,
        const QString& tartist, const QString& ttitle,
        const QString& year, const QString& genre,
        const QString& ext, const QString& basepath,
        bool fat_compatible, bool replacespaceswithunderscores,
        bool _2digitstracknum,
        bool overwrite_existing_files, bool is_first_track) {

  Q_UNUSED(is_first_track);

  PatternParser patternparser;
  targetFilename = ((basepath.right(1)=="/")?basepath:basepath+"/")+patternparser.parseFilenamePattern(profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_PATTERN_INDEX)).toString(),
        trackno, cdno, gindex, nooftracks, artist, title, tartist, ttitle, year, genre, ext, fat_compatible, replacespaceswithunderscores, _2digitstracknum);

  int lastSlash = targetFilename.lastIndexOf("/", -1);
  if (lastSlash == -1) {
    emit error(i18n("Can't find path \"%1\".", targetFilename), i18n("Please check your path (write access?)"));
    return FALSE;
  }
  QString targetPath = targetFilename.mid(0, lastSlash);
  QString targetStrippedFilename = targetFilename.mid(lastSlash+1);
  target_dir = targetPath;

  if (!p_mkdir(targetPath)) return FALSE;

  KDiskFreeSpaceInfo diskfreespace = KDiskFreeSpaceInfo::freeSpaceInfo(targetPath);
  quint64 free = diskfreespace.available() / 1024;
  if (free < 200*1024) {
    emit warning(i18n("Free space on \"%1\" is less than 200 MiB.", targetPath));
  }

  QFile *file = new QFile(targetFilename);
  if (file->exists()) {
    if (overwrite_existing_files) {
      emit warning(i18n("Warning! File \"%1\" already exists. Overwriting.", targetStrippedFilename));
    } else {
      emit warning(i18n("Warning! File \"%1\" already exists. Skipping.", targetStrippedFilename));

      cdda_model->setCustomDataPerTrack(trackno, "filename", targetFilename);
      cdda_model->setCustomDataPerTrack(trackno, "ripped", TRUE);

      targetFilename.clear();
    }
  }
  delete file;

  return TRUE;

}

bool Audex::construct_target_filename_for_singlefile(QString& targetFilename,
        int cdno, int nooftracks,
        const QString& artist, const QString& title,
        const QString& date, const QString& genre,
        const QString& ext, const QString& basepath,
        bool overwrite_existing_files) {

  PatternParser patternparser;
  targetFilename = ((basepath.right(1)=="/")?basepath:basepath+"/")+patternparser.parseSimplePattern(profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SF_NAME_INDEX)).toString(),
        cdno, nooftracks, artist, title, date, genre, ext, FALSE);

  int lastSlash = targetFilename.lastIndexOf("/", -1);
  if (lastSlash == -1) {
    emit error(i18n("Can't find path \"%1\".", targetFilename), i18n("Please check your path (write access?)"));
    return FALSE;
  }
  QString targetPath = targetFilename.mid(0, lastSlash);
  target_dir = targetPath;
  QString targetStrippedFilename = targetFilename.mid(lastSlash+1);
  QDir *dir = new QDir(targetPath);
  if (!dir->exists()) {
    if (!dir->mkpath(targetPath)) {
      emit error(i18n("Unable to create folder \"%1\".", targetPath), i18n("Please check your path (write access?)"));
      return FALSE;
    } else {
      emit info(i18n("Folder \"%1\" successfully created.", targetPath));
    }
  } else {
    emit warning(i18n("Folder \"%1\" already exists.", targetPath));
  }
  delete dir;

  KDiskFreeSpaceInfo diskfreespace = KDiskFreeSpaceInfo::freeSpaceInfo(targetPath);
  quint64 free = diskfreespace.available() / 1024;
  if (free < 800*1024) {
    emit warning(i18n("Free space on \"%1\" is less than 800 MiB.", targetPath));
  }

  QFile *file = new QFile(targetFilename);
  if (file->exists()) {
    if (overwrite_existing_files) {
      emit warning(i18n("Warning! File \"%1\" already exists. Overwriting.", targetStrippedFilename));
    } else {
      emit warning(i18n("Warning! File \"%1\" already exists. Skipping.", targetStrippedFilename));

      cdda_model->setCustomData("filename", targetFilename);

      targetFilename.clear();
    }
  }
  delete file;

  return TRUE;

}

bool Audex::check() {

  if (tmp_dir->error()) {
    slot_error(i18n("Temporary folder \"%1\" error.", tmp_dir->tmpPath()), i18n("Please check."));
    return FALSE;
  }

  quint64 free = tmp_dir->freeSpace() / 1024;
  if (free < 800*1024) {
    slot_warning(i18n("Free space on temporary folder \"%1\" is less than 800 MiB.", tmp_dir->tmpPathBase()));
  } else if (free < 200*1024) {
    slot_error(i18n("Temporary folder \"%1\" needs at least 200 MiB of free space.", tmp_dir->tmpPathBase()), i18n("Please free space or set another path."));
    return FALSE;
  }

  return TRUE;

}

void Audex::request_finish(bool successful) {

  if (!_finished) {
    _finished = TRUE;
    _finished_successful = successful;
  } else {
    return;
  }

  if (process_counter > 0) {

    encoder_wrapper->cancel();
    cdda_extract_thread->cancel();
    QTimer::singleShot(2000, this, SLOT(check_if_thread_still_running()));

  } else {

    execute_finish();

  }

}

void Audex::execute_finish() {

  if (Preferences::ejectCDTray()) {
    emit info(i18n("Eject CD tray"));
    cdda_model->eject();
  }

  bool overwrite = Preferences::overwriteExistingFiles();

  QStringList target_filename_list;
  for (int i = 0; i < cdda_model->rowCount(); ++i) {
    if (!cdda_model->isAudioTrack(i+1)) continue;
    if (!cdda_model->isTrackInSelection(i+1)) continue;
    if (!cdda_model->getCustomDataPerTrack(i+1, "ripped").toBool()) continue;
    target_filename_list.append(cdda_model->getCustomDataPerTrack(i+1, "filename").toString());
  }
  QString target_single_filename;
  if (p_single_file) {
    target_single_filename = cdda_model->customData("filename").toString();
  }

  QString co;
  if ((_finished_successful) && (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_INDEX)).toBool())) {

    //store the cover
    if (!cdda_model->isCoverEmpty()) {

      QImage image(cdda_model->coverImage());
      if (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_SCALE_INDEX)).toBool()) {
        QSize size = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_SIZE_INDEX)).toSize();
        image = image.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        kDebug() << QString("Cover scaled to %1x%2.").arg(size.width()).arg(size.height());
      }
      QString pattern = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_NAME_INDEX)).toString();
      QString format = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX)).toString();

      PatternParser patternparser;
      QString filename = patternparser.parseSimplePattern(pattern,
        cdda_model->cdNum(), cdda_model->numOfAudioTracks(),
        cdda_model->artist(), cdda_model->title(),
        QString("%1").arg(cdda_model->year()), cdda_model->genre(),
        format.toLower(), profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool());

      if (p_prepare_dir(filename, target_dir, overwrite)) {
        if (image.save(filename, format.toAscii().data())) {
          emit info(i18n("Cover \"%1\" successfully saved.", QFileInfo(filename).fileName()));
          co = filename;
        } else {
          emit error(i18n("Unable to save cover \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));
        }
      }

    }

  }

  QString pl;
  if ((_finished_successful) && (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_PL_INDEX)).toBool()) && (target_filename_list.count() > 0) && (!p_single_file)) {

    //create the playlist
    Playlist playlist;

    QString pattern = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_PL_NAME_INDEX)).toString();
    QString format = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_PL_FORMAT_INDEX)).toString();
    bool is_absFilePath = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX)).toBool();

    PatternParser patternparser;
    QString filename = patternparser.parseSimplePattern(pattern,
        cdda_model->cdNum(), cdda_model->numOfAudioTracks(),
        cdda_model->artist(), cdda_model->title(),
        QString("%1").arg(cdda_model->year()), cdda_model->genre(),
        format.toLower(), profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool());

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
          if (!cdda_model->isAudioTrack(i+1)) continue;
          if (!cdda_model->isTrackInSelection(i+1)) continue;
          if (!cdda_model->getCustomDataPerTrack(i+1, "ripped").toBool()) continue;
          PlaylistItem item;
          item.setFilename(cdda_model->getCustomDataPerTrack(i+1, "filename").toString());
          item.setArtist(cdda_model->data(cdda_model->index(i, CDDA_MODEL_COLUMN_ARTIST_INDEX)).toString());
          item.setTitle(cdda_model->data(cdda_model->index(i, CDDA_MODEL_COLUMN_TITLE_INDEX)).toString());
          item.setLength(cdda_model->lengthOfTrack(i+1));
          playlist.appendItem(item);
        }

        QString relFilePath;
        if (!is_absFilePath) {
          relFilePath = QFileInfo(filename).absoluteDir().absolutePath();
        }

        QTextStream out(&file);
        if (format == "M3U") {
          out << playlist.toM3U(relFilePath);
        } else if (format == "PLS") {
          out << playlist.toPLS(relFilePath);
        } else if (format == "XSPF") {
          out << playlist.toXSPF();
        }
        file.close();
        emit info(i18n("Playlist \"%1\" successfully created.", QFileInfo(filename).fileName()));
        pl = filename;

      } else {

        emit error(i18n("Unable to save playlist \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));

      }

    }

  }

  QString in;
  if ((_finished_successful) && (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_INF_INDEX)).toBool())) {

    PatternParser patternparser;
    QString filename = patternparser.parseSimplePattern(profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_INF_NAME_INDEX)).toString(),
        cdda_model->cdNum(), cdda_model->numOfAudioTracks(),
        cdda_model->artist(), cdda_model->title(),
        QString("%1").arg(cdda_model->year()), cdda_model->genre(),
        profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_INF_SUFFIX_INDEX)).toString(), profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool());

    if (p_prepare_dir(filename, target_dir, overwrite)) {

      QFile file(filename);
      if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);
        QStringList text = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_INF_TEXT_INDEX)).toStringList();
        patternparser.parseInfoText(text, cdda_model->artist(), cdda_model->title(),
          QString("%1").arg(cdda_model->year()), cdda_model->genre(),
          DiscIDCalculator::FreeDBId(cdda_model->discSignature()), p_size_of_all_files(target_filename_list), cdda_model->lengthOfAudioTracksInSelection(), cdda_model->numOfAudioTracksInSelection());
        out << text.join("\n");
        file.close();
        emit info(i18n("Info file \"%1\" successfully created.", QFileInfo(filename).fileName()));
        in = filename;
      } else {
        emit error(i18n("Unable to save info file \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));
      }

    }

  }

  QString hl;
  if ((_finished_successful) && (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_HL_INDEX)).toBool()) && (target_filename_list.count() > 0)) {

    QString pattern = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_HL_NAME_INDEX)).toString();
    QString format = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_HL_FORMAT_INDEX)).toString();

    PatternParser patternparser;
    QString filename = patternparser.parseSimplePattern(pattern,
        cdda_model->cdNum(), cdda_model->numOfAudioTracks(),
        cdda_model->artist(), cdda_model->title(),
        QString("%1").arg(cdda_model->year()), cdda_model->genre(),
        format.toLower(), profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool());

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
        if (fexists) out << "\n";
        Hashlist hashlist;
        if (format == "SFV") {
          out << hashlist.getSFV(target_filename_list).join("\n");
        } else if (format == "MD5") {
          out << hashlist.getMD5(target_filename_list).join("\n");
        }
        file.close();
        emit info(i18n("Hashlist \"%1\" successfully created.", QFileInfo(filename).fileName()));
        hl = filename;
      } else {
        emit error(i18n("Unable to save hashlist \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));
      }

    }

  }

  QString cs;
  if (
     (_finished_successful) &&
     (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_CUE_INDEX)).toBool()) &&
     (((target_filename_list.count() > 0) && !p_single_file) || p_single_file)
  ) {

    QString pattern = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_CUE_NAME_INDEX)).toString();

    PatternParser patternparser;
    QString filename = patternparser.parseSimplePattern(pattern,
        cdda_model->cdNum(), cdda_model->numOfAudioTracks(),
        cdda_model->artist(), cdda_model->title(),
        QString("%1").arg(cdda_model->year()), cdda_model->genre(),
        "cue", profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool());

    if (p_prepare_dir(filename, target_dir, overwrite)) {

      QFile file(filename);
      bool success = file.open(QFile::WriteOnly | QFile::Truncate);
      if (success) {
        QTextStream out(&file);
        CueSheetWriter cuesheetwriter(cdda_model);
        if (p_single_file) {
          out << cuesheetwriter.cueSheet(target_single_filename).join("\n");
        } else {
          out << cuesheetwriter.cueSheet(target_filename_list).join("\n");
        }
        file.close();
        emit info(i18n("Cue sheet \"%1\" successfully created.", QFileInfo(filename).fileName()));
        cs = filename;
      } else {
        emit error(i18n("Unable to save cue sheet \"%1\".", QFileInfo(filename).fileName()), i18n("Please check your path and permissions"));
      }

    }

  }


  if ((_finished_successful) && (Preferences::upload()) && (target_filename_list.count() > 0)) {

    QString targetpath = QFileInfo(target_filename_list.at(0)).absolutePath().mid(Preferences::basePath().length());

    QStringList files_to_transfer = target_filename_list;
    if (!co.isEmpty()) files_to_transfer << co;
    if (!pl.isEmpty()) files_to_transfer << pl;
    if (!in.isEmpty()) files_to_transfer << in;
    if (!hl.isEmpty()) files_to_transfer << hl;
    if (!cs.isEmpty()) files_to_transfer << cs;

    Upload upload(Preferences::url(), this);
    connect(&upload, SIGNAL(info(const QString&)), this, SLOT(slot_info(const QString&)));
    connect(&upload, SIGNAL(error(const QString&, const QString&)), this, SLOT(slot_error(const QString&, const QString&)));
    upload.upload(targetpath, files_to_transfer);

  }

  //flush temporary path
  if (!tmp_path.isEmpty()) {
    QDir tmp(tmp_path);
    QStringList files = tmp.entryList(QStringList() << "*", QDir::Files | QDir::NoDotAndDotDot);
    for (int i = 0; i < files.count(); ++i) {
      QFile::remove(tmp_path+files[i]);
      kDebug() << "Deleted temporary file" << tmp_path+files[i];
    }
  }

  emit finished(_finished_successful);

}

bool Audex::p_prepare_dir(QString& filename, const QString& targetDirIfRelative, const bool overwrite) {

  QString result;

  QFileInfo fileinfo(filename);
  if (fileinfo.isAbsolute()) {
    if (!p_mkdir(fileinfo.dir().absolutePath())) {
      return FALSE;
    } else {
      result = filename;
    }
  } else {
    if (!targetDirIfRelative.isEmpty()) {
      QDir dir(targetDirIfRelative);
      if (!dir.isReadable()) {
        emit error(i18n("Unable to open folder \"%1\".", targetDirIfRelative), i18n("Please check your path and permissions"));
        return FALSE;
      }
      result = targetDirIfRelative+"/"+filename;
    } else {
      result = filename;
    }
  }

  if (!overwrite) {
    QFileInfo info(result);
    if (info.exists()) {
      emit warning(i18n("Warning! File \"%1\" already exists. Skipping.", info.fileName()));
      return FALSE;
    }
  }

  filename = result;

  return TRUE;

}

bool Audex::p_mkdir(const QString& absoluteFilePath) {

  QDir dir(absoluteFilePath);
  if (dir.exists()) {
    if (!dir.isReadable()) {
      emit error(i18n("Unable to open folder \"%1\".", absoluteFilePath), i18n("Please check your path and permissions"));
      return FALSE;
    }
  } else {
    if (!dir.mkpath(absoluteFilePath)) {
      emit error(i18n("Unable to create folder \"%1\".", absoluteFilePath), i18n("Please check your path (write access?)"));
      return FALSE;
    } else {
      emit info(i18n("Folder \"%1\" successfully created.", absoluteFilePath));
    }
  }

  return TRUE;

}

qreal Audex::p_size_of_all_files(const QStringList& filenames) const {
  qreal size = .0f;
  for (int i = 0; i < filenames.count(); ++i) {
    QFileInfo info(filenames.at(i));
    size += info.size();
  }
  return size;
}
