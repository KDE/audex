/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
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
    explicit Upload(const QUrl &url, QObject *parent = 0);
    ~Upload();

    void upload(const QString &targetpath, const QStringList &filelist);

signals:
    void error(const QString &message, const QString &solution = QString());
    void warning(const QString &message);
    void info(const QString &message);

private:
    QUrl base_url;
};

#endif
