/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QUrl>

#include <optional>

#include "metadata/metadata.h"
#include "online/coverartarchive.h"
#include "online/responsecache.h"

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;

namespace Audex
{

// Downloads cover images, typically from the Cover Art Archive
// (https://coverartarchive.org/release/<mbid>/front). The archive answers with
// a redirect to the actual image; thumbnails are available as front-250,
// front-500 and front-1200.
class CoverArtFetcher : public QObject
{
    Q_OBJECT

public:
    enum class Size {
        Original = 0,
        Small = 250,
        Medium = 500,
        Large = 1200,
    };
    Q_ENUM(Size)

    explicit CoverArtFetcher(QNetworkAccessManager *network, QObject *parent = nullptr);

    void setUserAgent(const QString &userAgent);
    void setMaximumBytes(qint64 bytes); // default 25 MiB
    // Images kept on disk; as with the metadata providers, with preferCache a
    // kept image is taken without asking the archive. Listings are always
    // asked for: the archive may have new images.
    void setCache(const std::optional<ResponseCache> &cache);
    void setPreferCache(bool prefer);

    // Returns a request id; finished() is always emitted asynchronously.
    int fetch(const QUrl &url, Size size = Size::Original);
    // The images the archive has for a release or release group
    // (https://coverartarchive.org/release/<mbid>); listingFinished() is
    // always emitted asynchronously, with an empty listing if there are none.
    int fetchListing(const QUrl &url);
    void cancel(int requestId);
    bool isRunning(int requestId) const;

    // ".../front" -> ".../front-500" for Cover Art Archive URLs, otherwise unchanged
    static QUrl sizedUrl(const QUrl &url, Size size);

Q_SIGNALS:
    void progress(int requestId, qint64 received, qint64 total);
    // cover is null if nothing was found or on error; "no cover" is not an error
    void finished(int requestId, const Audex::Metadata::CoverArt &cover, const QString &error);
    void listingFinished(int requestId, const Audex::CoverArtListing &listing, const QString &error);

private:
    QPointer<QNetworkAccessManager> m_network;
    QNetworkRequest request(const QUrl &url) const;
    std::optional<QByteArray> cached(const QUrl &url) const;
    void keep(const QUrl &url, const QByteArray &data);
    std::optional<ResponseCache> m_cache;
    bool m_preferCache = true;
    QSet<int> m_fromCache; // answered from the cache, not yet emitted
    QString m_userAgent;
    qint64 m_maximumBytes = 25 * 1024 * 1024;
    QHash<int, QPointer<QNetworkReply>> m_replies;
    int m_nextId = 1;
};

}
