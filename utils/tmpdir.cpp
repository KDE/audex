/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tmpdir.h"

#include <QDebug>
#include <QStandardPaths>

TmpDir::TmpDir(const QString &appName, const QString &sub)
    : QObject()
{
    QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::TempLocation);
    p_tmp_path_base = dirs.size() ? dirs[0] : "/var/tmp/";
    qDebug() << "Found temporary path" << p_tmp_path_base;

    p_error = false;

    PID pid;
    p_tmp_path = p_tmp_path_base;
    if (p_tmp_path.right(1) != "/")
        p_tmp_path += "/";
    p_tmp_path += appName + '.' + QString("%1").arg(pid.getPID()) + '/';
    p_tmp_path_app = p_tmp_path;
    if (!sub.isEmpty()) {
        p_tmp_path += sub + '/';
    }
    qDebug() << "Temporary folder in use:" << p_tmp_path;
    p_error = !p_create_dir(p_tmp_path);
}

TmpDir::~TmpDir()
{
    // do we have a sub component in the path?
    if (p_tmp_path_app != p_tmp_path) {
        if (p_remove_dir(p_tmp_path)) {
            qDebug() << QString("Deleting temporary folder \"%1\"").arg(p_tmp_path);
        } else {
            qDebug() << QString("Deleting temporary folder \"%1\" failed").arg(p_tmp_path);
        }
    }

    QDir dir(p_tmp_path_app);
    if ((dir.exists()) && (!dir.rmdir(p_tmp_path_app))) {
        qDebug() << QString("Temporary folder \"%1\" not removed yet.").arg(p_tmp_path_app);
    }
}

const QString TmpDir::tmpPath()
{
    p_error = !p_create_dir(p_tmp_path);
    return p_tmp_path;
}

quint64 TmpDir::freeSpace() const
{
    QStorageInfo diskfreespace(p_tmp_path);
    return diskfreespace.bytesAvailable();
}

bool TmpDir::p_create_dir(const QString &dirName)
{
    QDir *dir = new QDir(dirName);
    if (!dir->exists()) {
        if (!dir->mkpath(dirName)) {
            return false;
        }
    }

    return true;
}

bool TmpDir::p_remove_dir(const QString &dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH (QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = p_remove_dir(info.absoluteFilePath());
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }
            if (!result) {
                return result;
            }
        }

        result = dir.rmdir(dirName);
    }

    return result;
}
