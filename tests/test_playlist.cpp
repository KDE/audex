/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "utils/playlist.h"

#include <QDir>
#include <QFile>

using namespace Qt::StringLiterals;

AUDEX_TEST("playlist: absolute and relative M3U paths")
{
    const QString root = QDir::temp().filePath(u"audex-playlist-test"_s);
    const QString playlistDir = QDir(root).filePath(u"playlists"_s);
    const QString trackFile = QDir(root).filePath(u"audio/01 - Track.flac"_s);

    Playlist playlist;
    PlaylistItem item;
    item.setFilename(trackFile);
    item.setArtist(u"Artist"_s);
    item.setTitle(u"Track"_s);
    item.setLength(210);
    playlist.appendItem(item);

    const QByteArray header = "#EXTM3U\n#EXTINF:210,Artist - Track\n";
    const QByteArray relative = header + QFile::encodeName(QDir(playlistDir).relativeFilePath(trackFile)) + '\n';
    const QByteArray absolute = header + QFile::encodeName(trackFile) + '\n';

    AUDEX_EQUAL_DATA(t, playlist.toM3U(playlistDir, true), relative);
    AUDEX_EQUAL_DATA(t, playlist.toM3U(QString(), true), absolute);
}
