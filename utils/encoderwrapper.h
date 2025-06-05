/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QImage>
#include <QRegularExpression>
#include <QString>

#include <KLocalizedString>
#include <KProcess>

#include "utils/schemeparser.h"

namespace Audex
{

class EncoderWrapper : public QObject
{
    Q_OBJECT

public:
    explicit EncoderWrapper(QObject *parent = nullptr,
                            const QString &commandScheme = "",
                            const QString &encoderName = "",
                            const bool deleteFractionFiles = true);
    ~EncoderWrapper() override;

    bool isProcessing();
    const QStringList &log();

public Q_SLOTS:
    bool encode(int n,
                int cdno,
                int trackoffset,
                int nooftracks,
                const QString &artist,
                const QString &album,
                const QString &tartist,
                const QString &ttitle,
                const QString &genre,
                const QString &date,
                const QString &isrc,
                const QString &suffix,
                const QImage &cover,
                const QString &tmppath,
                const QString &input,
                const QString &output);
    void cancel();

private Q_SLOTS:
    void parseOutput();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError err);

Q_SIGNALS:
    void progress(int percent_of_track);
    void finished();

    void error(const QString &message, const QString &details = QString());
    void warning(const QString &message);
    void info(const QString &message);

private:
    QString command_scheme;
    QString encoder_name;
    bool delete_fraction_files;

    QString encoder;
    QStringList p_log;

    QString processing_filename;

    bool termination;
    int processing;

    KProcess proc;

    int not_found_counter;
};

}
