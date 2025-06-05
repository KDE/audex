/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QObject>

#include <KIO/CopyJob>
#include <KIO/Job>
#include <KIO/MkdirJob>
#include <KIO/SimpleJob>
#include <KLocalizedString>

namespace Audex
{

class Upload : public QObject
{
    Q_OBJECT
public:
    explicit Upload(const QUrl &url, QObject *parent = nullptr);

    void upload(const QString &targetpath, const QStringList &filelist);

Q_SIGNALS:
    void error(const QString &message, const QString &solution = QString());
    void warning(const QString &message);
    void info(const QString &message);

private:
    QUrl base_url;
};

}
