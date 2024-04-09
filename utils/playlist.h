/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QDir>
#include <QFileInfo>
#include <QDomDocument>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QRegularExpression>

#include <KLocalizedString>

namespace Audex
{

class PlaylistItem
{
public:
    PlaylistItem()
    {
        p_length = 0;
    }
    PlaylistItem(const PlaylistItem &other)
    {
        p_filename = other.p_filename;
        p_artist = other.p_artist;
        p_title = other.p_title;
        p_length = other.p_length;
    }
    PlaylistItem &operator=(const PlaylistItem &other)
    {
        p_filename = other.p_filename;
        p_artist = other.p_artist;
        p_title = other.p_title;
        p_length = other.p_length;
        return *this;
    }
    ~PlaylistItem()
    {
    }

    bool operator==(const PlaylistItem &other) const
    {
        return (p_filename == other.p_filename && p_title == other.p_title && p_artist == other.p_artist && p_length == other.p_length);
    }

    void clear()
    {
        p_filename.clear();
        p_artist.clear();
        p_title.clear();
        p_length = 0;
    }

    void setFilename(const QString &filename)
    {
        p_filename = filename;
    }
    const QString filename()
    {
        return p_filename;
    }
    void setArtist(const QString &artist)
    {
        p_artist = artist;
    }
    const QString artist()
    {
        return p_artist;
    }
    void setTitle(const QString &title)
    {
        p_title = title;
    }
    const QString title()
    {
        return p_title;
    }
    void setLength(const int length)
    {
        p_length = length;
    }
    int length()
    {
        return p_length;
    }

private:
    QString p_filename;
    QString p_artist;
    QString p_title;
    int p_length; // sec
};

typedef QList<PlaylistItem> PlaylistItemList;

class Playlist
{
public:
    Playlist();
    explicit Playlist(const QByteArray &playlist);
    ~Playlist();

    void addPlaylist(const QByteArray &playlist);

    void clear();

    void appendItem(const PlaylistItem &item);

    // if playlistPath is set, then filename paths will be relative to playlistPath
    QByteArray toM3U(const QString &playlistPath = "", const bool utf8 = false) const;
    QByteArray toPLS(const QString &playlistPath = "", const bool utf8 = false) const;
    QByteArray toXSPF() const;

private:
    PlaylistItemList playlist_item_list;

    // guess the playlist format: m3u, pls, xspf
    const QString playlist_format(const QByteArray &playlist);

    void add_M3U(const QByteArray &playlist);
    void add_PLS(const QByteArray &playlist);
    void add_XSPF(const QByteArray &playlist);

    const PlaylistItem parse_m3u_metadata_line(const QString &line);
};

}

#endif
