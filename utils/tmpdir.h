/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TMPDIR_H
#define TMPDIR_H

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QObject>
#include <QStorageInfo>

#include "utils/pid.h"

class TmpDir : public QObject
{
    Q_OBJECT

public:
    explicit TmpDir(const QString &appName, const QString &sub = "");
    ~TmpDir() override;

    const QString tmpPath();
    inline const QString tmpPathBase() const
    {
        return p_tmp_path_base;
    }
    inline bool error() const
    {
        return p_error;
    }

    quint64 freeSpace() const;

private:
    QString p_tmp_path_base; // e.g. /var/tmp
    QString p_tmp_path_app;  // e.g. /var/tmp/audex.1234
    QString p_tmp_path;      // e.g. /car/tmp/audex.1234/sub
    bool p_error;

    bool p_create_dir(const QString &dirName);
    bool p_remove_dir(const QString &dirName);
};

#endif
