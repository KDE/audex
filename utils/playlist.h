/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

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
    ~Playlist();

    void appendItem(const PlaylistItem &item);

    // if playlistPath is set, then filename paths will be relative to playlistPath
    QByteArray toM3U(const QString &playlistPath = "", const bool utf8 = false) const;

private:
    PlaylistItemList p_playlist;
};
