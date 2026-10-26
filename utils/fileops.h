/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QFile>
#include <QFileInfo>
#include <QString>

#include <filesystem>
#include <system_error>

namespace Audex
{

inline std::filesystem::path nativePath(const QString &path)
{
#ifdef Q_OS_WIN
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(QFile::encodeName(path).constData());
#endif
}

inline bool replaceFile(const QString &source, const QString &target)
{
    std::error_code error;
    std::filesystem::rename(nativePath(source), nativePath(target), error);
    return !error;
}

inline bool backupFile(const QString &source, const QString &target)
{
    if (!QFileInfo(source).isSymLink()) {
        std::error_code error;
        std::filesystem::create_hard_link(nativePath(source), nativePath(target), error);
        if (!error)
            return true;
    }
    return QFile::copy(source, target);
}

}
