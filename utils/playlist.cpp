/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "playlist.h"

#include <KLocalizedString>

#include <QDir>
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

QByteArray Playlist::toM3U(const QString &playlistPath, const bool utf8) const
{
    QStringList playlist;
    playlist.append("#EXTM3U");

    for (int i = 0; i < p_playlist.count(); ++i) {
        PlaylistItem pi = p_playlist[i];
        if (pi.filename().isEmpty())
            continue;

        if (!pi.artist().isEmpty()) {
            playlist.append(QString("#EXTINF:%1,%2 - %3").arg(QString::number(pi.length()), pi.artist(), pi.title()));
        } else {
            playlist.append(QString("#EXTINF:%1,%2").arg(QString::number(pi.length()), pi.title()));
        }
        if (!playlistPath.isEmpty()) {
            QDir dir(playlistPath);
            playlist.append(dir.relativeFilePath(pi.filename()));
        } else {
            playlist.append(pi.filename());
        }
    }

    if (utf8)
        return playlist.join("\n").append("\n").toUtf8();
    else
        return playlist.join("\n").append("\n").toLatin1();
}
