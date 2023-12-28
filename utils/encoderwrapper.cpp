/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "encoderwrapper.h"

#include <QDebug>

EncoderWrapper::EncoderWrapper(QObject *parent, const QString &commandPattern, const QString &encoderName, const bool deleteFractionFiles)
    : QObject(parent)
{
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

EncoderWrapper::~EncoderWrapper()
{
}

bool EncoderWrapper::encode(int n,
                            int cdno,
                            int trackoffset,
                            int nooftracks,
                            const QString &artist,
                            const QString &album,
                            const QString &tartist,
                            const QString &ttitle,
                            const QString &genre,
                            const QString &date,
                            const QString &suffix,
                            const QImage &cover,
                            bool fat_compatible,
                            const QString &tmppath,
                            const QString &input,
                            const QString &output)
{
    if (!processing)
        processing = 1;
    else
        return false;
    termination = false;

    if (command_pattern.isEmpty()) {
        Q_EMIT error(i18n("Command pattern is empty."));
        return false;
    }

    PatternParser patternparser;
    QString command = patternparser.parseCommandPattern(command_pattern, input, output, n, cdno, trackoffset, nooftracks, artist, album, tartist, ttitle, date, genre, suffix, cover, fat_compatible, tmppath, encoder_name);

    qDebug() << "executing command " << command;
    proc.setShellCommand(command);
    proc.start();
    proc.waitForStarted();

    processing_filename = output;

    Q_EMIT info(i18n("Encoding track %1...", n));

    return true;
}

void EncoderWrapper::cancel()
{
    if (!processing)
        return;

    // we need to suppress normal error messages, because with a cancel the user known what he does
    termination = true;
    proc.terminate();

    if (delete_fraction_files) {
        QFile file(processing_filename);
        if (file.exists()) {
            file.remove();
            Q_EMIT warning(i18n("Deleted partial file \"%1\".", processing_filename.mid(processing_filename.lastIndexOf("/") + 1)));
            qDebug() << "deleted partial file" << processing_filename;
        }
    }

    Q_EMIT error(i18n("User canceled encoding."));
    qDebug() << "Interrupt encoding.";
}

bool EncoderWrapper::isProcessing()
{
    return (processing > 0);
}

const QStringList &EncoderWrapper::protocol()
{
    return _protocol;
}

void EncoderWrapper::parseOutput()
{
    QByteArray rawoutput = proc.readAllStandardError();
    if (rawoutput.size() == 0)
        rawoutput = proc.readAllStandardOutput();
    bool found = false;
    if (rawoutput.size() > 0) {
        QString output(rawoutput);
        QStringList list = output.trimmed().split('\n');
        _protocol << list;
        for (int i = 0; i < list.count(); ++i) {
            if (list.at(i).contains('%')) {
                QString line = list.at(i);
                static const QRegularExpression regex("\\d+[,.]?\\d*\\%");
                int startPos = line.indexOf(regex);
                if (startPos == -1)
                    continue;
                QString p = line.mid(startPos);
                p = p.left(p.indexOf('%'));
                bool conversionSuccessful = false;
                double percent = p.toDouble(&conversionSuccessful);
                if ((conversionSuccessful) && (percent >= 0) && (percent <= 100)) {
                    Q_EMIT progress((int)percent);
                    found = true;
                    not_found_counter = 0;
                }
            }
        }
    }
    if (!found) {
        if (not_found_counter > 5)
            Q_EMIT progress(-1);
        else
            ++not_found_counter;
    }
}

void EncoderWrapper::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    processing = 0;
    if (termination) {
        Q_EMIT finished();
        return;
    }
    if ((exitStatus == QProcess::NormalExit) && (exitCode == 0)) {
        Q_EMIT info(i18n("Encoding OK (\"%1\").", processing_filename));
    } else {
        Q_EMIT error(i18n("An error occurred while encoding file \"%1\".", processing_filename), i18n("Please check your profile."));
    }
    Q_EMIT finished();
    qDebug() << "encoding finished.";
}

void EncoderWrapper::processError(QProcess::ProcessError err)
{
    if (termination)
        return;
    switch (err) {
    case QProcess::FailedToStart:
        Q_EMIT error(i18n("%1 failed to start.", encoder), i18n("Either it is missing, or you may have insufficient permissions to invoke the program."));
        break;
    case QProcess::Crashed:
        Q_EMIT error(i18n("%1 crashed some time after starting successfully.", encoder), i18n("Please check your profile."));
        break;
    case QProcess::Timedout:
        Q_EMIT error(i18n("%1 timed out. This should not happen.", encoder), i18n("Please check your profile."));
        break;
    case QProcess::WriteError:
        Q_EMIT error(i18n("An error occurred when attempting to write to %1.", encoder), i18n("For example, the process may not be running, or it may have closed its input channel."));
        break;
    case QProcess::ReadError:
        Q_EMIT error(i18n("An error occurred when attempting to read from %1.", encoder), i18n("For example, the process may not be running."));
        break;
    case QProcess::UnknownError:
        Q_EMIT error(i18n("An unknown error occurred to %1. This should not happen.", encoder), i18n("Please check your profile."));
        break;
    }
    Q_EMIT finished();
    qDebug() << "encoding finished.";
}
