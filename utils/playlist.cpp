/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "playlist.h"

#include <KLocalizedString>

#include <QDir>
#include <QFile>
#include <QStringList>

Playlist::Playlist()
{
}

Playlist::~Playlist()
{
}

void Playlist::appendItem(const PlaylistItem &item)
{
    p_playlist.append(item);
}

// The paths are written as the file system stores them (as QFile does), so a
// player finds the files whatever the setting; only the titles are Latin-1
// without UTF-8 (the classic M3U), characters outside it become "?".
QByteArray Playlist::toM3U(const QString &playlistPath, const bool utf8) const
{
    const auto text = [utf8](const QString &line) {
        return utf8 ? line.toUtf8() : line.toLatin1();
    };

    QByteArray playlist = "#EXTM3U\n";
    for (PlaylistItem pi : p_playlist) { // a copy: the getters are not const
        if (!pi.artist().isEmpty())
            playlist += text(QString("#EXTINF:%1,%2 - %3").arg(QString::number(pi.length()), pi.artist(), pi.title()));
        else
            playlist += text(QString("#EXTINF:%1,%2").arg(QString::number(pi.length()), pi.title()));
        playlist += '\n';
        const QString path = playlistPath.isEmpty() ? pi.filename() : QDir(playlistPath).relativeFilePath(pi.filename());
        playlist += QFile::encodeName(path);
        playlist += '\n';
    }

    return playlist;
}
