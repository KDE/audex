/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <QUrl>

#include <optional>

namespace Audex
{

// Answers of online services on disk, so that a disc inserted again needs no
// network: MusicBrainz and gnudb answers, Cover Art Archive images.
// The key is the request URL, which holds the whole table of contents (or the
// MusicBrainz disc ID, an ID of an entry), so discs whose CDDB IDs collide are
// never mixed up. Personal parts (the gnudb email address) are left out.
// Only answers that found something are kept: a disc missing today may be
// entered tomorrow.
class ResponseCache
{
public:
    explicit ResponseCache(const QString &directory = defaultDirectory());

    static QString defaultDirectory(); // ~/.cache/audex/online

    static constexpr int NoLimit = -1;

    // 0 days: nothing is kept or found; NoLimit: kept until cleared
    void setMaximumAge(int days);
    int maximumAge() const;
    // the oldest answers go first; default 200 MiB, NoLimit: no limit
    void setMaximumSize(qint64 bytes);

    // service: "musicbrainz", "gnudb", "coverart", one folder each
    std::optional<QByteArray> find(const QString &service, const QUrl &url) const;
    void insert(const QString &service, const QUrl &url, const QByteArray &data);

    qint64 size() const; // bytes on disk
    void clear();

    static QString key(const QUrl &url);

private:
    QString path(const QString &service, const QUrl &url) const;
    void trim() const;

    QString m_directory;
    int m_maximumAge = 30;
    qint64 m_maximumSize = qint64(200) * 1024 * 1024;
};

}
