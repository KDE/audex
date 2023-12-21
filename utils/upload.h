/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UPLOAD_H
#define UPLOAD_H

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QObject>

#include <KIO/CopyJob>
#include <KIO/Job>
#include <KIO/SimpleJob>
#include <KLocalizedString>

class Upload : public QObject
{
    Q_OBJECT
public:
    explicit Upload(const QUrl &url, QObject *parent = nullptr);
    ~Upload() override;

    void upload(const QString &targetpath, const QStringList &filelist);

Q_SIGNALS:
    void error(const QString &message, const QString &solution = QString());
    void warning(const QString &message);
    void info(const QString &message);

private:
    QUrl base_url;
};

#endif
