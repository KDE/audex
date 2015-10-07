/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <http://userbase.kde.org/Audex>
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

#include "cddaextractthread.h"

static CDDAExtractThread* aet = 0;

void paranoiaCallback(long sector, int status) {
  aet->createStatus(sector, status);
}

CDDAExtractThread::CDDAExtractThread(QObject* parent, CDDAParanoia *_paranoia) : QThread(parent) {

  paranoia = _paranoia;
  if (!paranoia) {
    kDebug() << "Paranoia object not found. low mem?";
    emit error(i18n("Internal device error."), i18n("Check your device and make a bug report."));
    return;
  }
  connect(paranoia, SIGNAL(error(const QString&, const QString&)), this, SLOT(slot_error(const QString&, const QString&)));

  overall_sectors_read = 0;
  paranoia_mode = 3;
  paranoia_retries = 20;
  never_skip = true;
  sample_offset = 0;
  sample_offset_done = false;
  track = 1;
  b_interrupt = false;
  b_error = false;
  read_error = false;
  scratch_detected = false;

}

CDDAExtractThread::~CDDAExtractThread() {

}

void CDDAExtractThread::start() {
  QThread::start();
}

void CDDAExtractThread::run() {

  if (!paranoia) return;

  if (b_interrupt) return;

  b_interrupt = false;
  b_error = false;

  if ((sample_offset) && (!sample_offset_done)) {
    paranoia->sampleOffset(sample_offset);
    sample_offset_done = true;
  }

  if (track == 0) {
    first_sector = paranoia->firstSectorOfDisc();
    last_sector = paranoia->lastSectorOfDisc();
  } else {
    first_sector = paranoia->firstSectorOfTrack(track);
    last_sector = paranoia->lastSectorOfTrack(track);
  }

  if (first_sector < 0 || last_sector < 0) {
    emit info(i18n("Extracting finished."));
    return;
  }

  kDebug() << "Sectors to read: " << QString("%1").arg(last_sector-first_sector);

  //status variable
  last_read_sector = 0;
  overlap = 0;
  read_sectors = 0;

  //track length
  sectors_all = last_sector-first_sector;
  sectors_read = 0;

  paranoia->setParanoiaMode(paranoia_mode);
  paranoia->setNeverSkip(never_skip);
  paranoia->setMaxRetries(paranoia_retries);

  paranoia->paranoiaSeek(first_sector, SEEK_SET);
  current_sector = first_sector;

  if (track > 0) {
    QString min = QString("%1").arg((sectors_all / 75) / 60, 2, 10, QChar('0'));
    QString sec = QString("%1").arg((sectors_all / 75) % 60, 2, 10, QChar('0'));
    emit info(i18n("Ripping track %1 (%2:%3)...", track, min, sec));
  } else {
    emit info(i18n("Ripping whole CD as single track."));
  }
  extract_protocol.append(i18n("Start reading track %1 with %2 sectors", track, sectors_all));

  while (current_sector <= last_sector) {

    if (b_interrupt) {
      kDebug() << "Interrupt reading.";
      break;
    }

    //let the global paranoia callback have access to this
    //to emit signals
    aet = this;

    int16_t* buf = paranoia->paranoiaRead(paranoiaCallback);

    if (0 == buf) {

      kDebug() << "Unrecoverable error in paranoia_read (sector " << current_sector << ")";
      b_error = true;
      break;

    } else {

      current_sector++;
      QByteArray a((char*)buf, CD_FRAMESIZE_RAW);
      emit output(a);
      a.clear();

      sectors_read++;
      overall_sectors_read++;
      float fraction = 0.0f;
      if (sectors_all > 0) fraction = (float)sectors_read / (float)sectors_all;
      emit progress((int)(100.0f*fraction), current_sector, overall_sectors_read);

    }

  }

  if (b_interrupt)
    emit error(i18n("User canceled extracting."));

  if (b_error)
    emit error(i18n("An error occurred while ripping track %1.", track));

  if ((!b_interrupt) && (!b_error)) {
    if (track > 0) {
      emit info(i18n("Ripping OK (Track %1).", track));
    } else {
      emit info(i18n("Ripping OK."));
    }
  }

  kDebug () << "Reading finished.";
  extract_protocol.append(i18n("Reading finished"));

}

void CDDAExtractThread::cancel() {
  b_interrupt = true;
}

bool CDDAExtractThread::isProcessing() {
  return !(b_interrupt || !isRunning());
}

const QStringList& CDDAExtractThread::protocol() {
  return extract_protocol;
}

void CDDAExtractThread::slot_error(const QString& message, const QString& details) {
  emit error(message, details);
}

void CDDAExtractThread::createStatus(long sector, int status) {

  sector /= CD_FRAMESIZE_RAW/2;
  QString tp_min = QString("%1").arg((current_sector/75)/60, 2, 10, QChar('0'));
  QString tp_sec = QString("%1").arg((current_sector/75)%60, 2, 10, QChar('0'));

  switch (status) {
  case -1:
    break;
  case -2:
    break;
  case PARANOIA_CB_READ:
    //no problem
    last_read_sector = sector;  //this seems to be rather useless
    read_sectors++;
    read_error = false;
    scratch_detected = false;
    break;
  case PARANOIA_CB_VERIFY:
    //kDebug() << "Verifying jitter";
    break;
  case PARANOIA_CB_FIXUP_EDGE:
    kDebug() << "Fixed edge jitter";
    extract_protocol.append(i18n("Fixed edge jitter (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    break;
  case PARANOIA_CB_FIXUP_ATOM:
    kDebug() << "Fixed atom jitter";
    extract_protocol.append(i18n("Fixed atom jitter (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    break;
  case PARANOIA_CB_SCRATCH:
    //scratch detected
    kDebug() << "Scratch detected";
    if (!scratch_detected) { scratch_detected = true; warning(i18n("Scratch detected (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec)); }
    extract_protocol.append(i18n("SCRATCH DETECTED (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    break;
  case PARANOIA_CB_REPAIR:
    kDebug() << "Repair";
    extract_protocol.append(i18n("Repair (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    break;
  case PARANOIA_CB_SKIP:
    //skipped sector
    kDebug() << "Skip";
    warning(i18n("Skip sectors (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    extract_protocol.append(i18n("SKIP (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    break;
  case PARANOIA_CB_DRIFT:
    kDebug() << "Drift";
    extract_protocol.append(i18n("Drift (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    break;
  case PARANOIA_CB_BACKOFF:
    kDebug() << "Backoff";
    extract_protocol.append(i18n("Backoff (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    break;
  case PARANOIA_CB_OVERLAP:
    //sector does not seem to contain the current
    //sector but the amount of overlapped data
    //kDebug() << "overlap.";
    overlap = sector;
    break;
  case PARANOIA_CB_FIXUP_DROPPED:
    kDebug() << "Fixup dropped";
    extract_protocol.append(i18n("Fixup dropped (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    break;
  case PARANOIA_CB_FIXUP_DUPED:
    kDebug() << "Fixup duped";
    extract_protocol.append(i18n("Fixup duped (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    break;
  case PARANOIA_CB_READERR:
    kDebug() << "Read error";
    if (!read_error) { read_error = true; warning(i18n("Read error detected (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec)); }
    extract_protocol.append(i18n("READ ERROR (absolute sector %1, relative sector %2, track time pos %3:%4)", sector, current_sector, tp_min, tp_sec));
    break;
  }

}
