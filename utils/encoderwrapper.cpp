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

#include "encoderwrapper.h"

EncoderWrapper::EncoderWrapper(QObject* parent, const QString& commandPattern, const QString& encoderName, const bool deleteFractionFiles) : QObject(parent) {

  command_pattern = commandPattern;
  encoder_name = encoderName;
  delete_fraction_files = deleteFractionFiles;

  connect(&proc, SIGNAL(readyReadStandardError()), this, SLOT(parseOutput()));
  connect(&proc, SIGNAL(readyReadStandardOutput()), this, SLOT(parseOutput()));
  connect(&proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
  connect(&proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));

  proc.setOutputChannelMode(KProcess::SeparateChannels);
  proc.setReadChannel(KProcess::StandardError);

  termination = false;
  processing = 0;

  not_found_counter = 0;

}

EncoderWrapper::~EncoderWrapper() {

}

bool EncoderWrapper::encode(int n,
	int cdno, int trackoffset, int nooftracks,
	const QString& artist, const QString& album,
	const QString& tartist, const QString& ttitle,
	const QString& genre, const QString& date, const QString& suffix, CachedImage *cover,
	bool fat_compatible, const QString& tmppath,
	const QString& input, const QString& output) {

  if (!processing) processing = 1; else return false;
  termination = false;

  if (command_pattern.isEmpty()) { emit error(i18n("Command pattern is empty.")); return false; }

  PatternParser patternparser;
  QString command = patternparser.parseCommandPattern(command_pattern, input, output, n, cdno, trackoffset, nooftracks, artist, album, tartist, ttitle, date, genre, suffix, cover, fat_compatible, tmppath, encoder_name);
 
  kDebug() << "executing command " << command;
  proc.setShellCommand(command);
  proc.start();
  proc.waitForStarted();

  processing_filename = output;

  emit info(i18n("Encoding track %1...", n));

  return true;

}

void EncoderWrapper::cancel() {

  if (!processing) return;

  //we need to suppress normal error messages, because with a cancel the user known what he does
  termination = true;
  proc.terminate();

  if (delete_fraction_files) {
    QFile file(processing_filename);
    if (file.exists()) {
      file.remove();
      emit warning(i18n("Deleted partial file \"%1\".", processing_filename.mid(processing_filename.lastIndexOf("/")+1)));
      kDebug() << "deleted partial file" << processing_filename;
    }
  }

  emit error(i18n("User canceled encoding."));
  kDebug() << "Interrupt encoding.";

}

bool EncoderWrapper::isProcessing() {
  return (processing>0);
}

const QStringList& EncoderWrapper::protocol() {
  return _protocol;
}

void EncoderWrapper::parseOutput() {

  QByteArray rawoutput = proc.readAllStandardError();
  if (rawoutput.size() == 0) rawoutput = proc.readAllStandardOutput();
  bool found = false;
  if (rawoutput.size() > 0) {
    QString output(rawoutput); QStringList list = output.trimmed().split("\n");
    _protocol << list;
    for (int i = 0; i < list.count(); ++i) {
      if (list.at(i).contains('%')) {
        QString line = list.at(i);
        int startPos = line.indexOf(QRegExp("\\d+[,.]?\\d*\\%"));
        if (startPos == -1) continue;
        QString p = line.mid(startPos);
        p = p.left(p.indexOf('%'));
        bool conversionSuccessful = false;
        double percent = p.toDouble(&conversionSuccessful);
        if ((conversionSuccessful) && (percent >= 0) && (percent <= 100)) {
          emit progress((int)percent);
          found = true;
	  not_found_counter = 0;
        }
      }
    }
  }
  if (!found) {
    if (not_found_counter > 5) emit progress(-1); else ++not_found_counter;
  }

}

void EncoderWrapper::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  processing = 0;
  if (termination) { emit finished(); return; }
  if ((exitStatus == QProcess::NormalExit) && (exitCode==0)) {
    emit info(i18n("Encoding OK (\"%1\").", processing_filename));
  } else {
    emit error(i18n("An error occurred while encoding file \"%1\".", processing_filename),
	i18n("Please check your profile."));
  }
  emit finished();
  kDebug() << "encoding finished.";
}

void EncoderWrapper::processError(QProcess::ProcessError err) {
  if (termination) return;
  switch (err) {
    case QProcess::FailedToStart :
      emit error(i18n("%1 failed to start.", encoder), i18n("Either it is missing, or you may have insufficient permissions to invoke the program.")); break;
    case QProcess::Crashed :
      emit error(i18n("%1 crashed some time after starting successfully.", encoder), i18n("Please check your profile.")); break;
    case QProcess::Timedout :
      emit error(i18n("%1 timed out. This should not happen.", encoder), i18n("Please check your profile.")); break;
    case QProcess::WriteError :
      emit error(i18n("An error occurred when attempting to write to %1.", encoder), i18n("For example, the process may not be running, or it may have closed its input channel.")); break;
    case QProcess::ReadError :
      emit error(i18n("An error occurred when attempting to read from %1.", encoder), i18n("For example, the process may not be running.")); break;
    case QProcess::UnknownError :
      emit error(i18n("An unknown error occurred to %1. This should not happen.", encoder), i18n("Please check your profile.")); break;
  }
  emit finished();
  kDebug() << "encoding finished.";
}
