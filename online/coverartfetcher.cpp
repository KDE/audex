/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "coverartfetcher.h"

#include "musicbrainzprovider.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

using namespace Qt::StringLiterals;

namespace Audex
{

CoverArtFetcher::CoverArtFetcher(QNetworkAccessManager *network, QObject *parent)
    : QObject(parent)
    , m_network(network)
    , m_userAgent(MusicBrainzProvider::defaultUserAgent())
{
    qRegisterMetaType<Audex::Metadata::CoverArt>();
    qRegisterMetaType<Audex::CoverArtListing>();
}

void CoverArtFetcher::setUserAgent(const QString &userAgent)
{
    m_userAgent = userAgent;
}

void CoverArtFetcher::setMaximumBytes(qint64 bytes)
{
    m_maximumBytes = bytes;
}

QUrl CoverArtFetcher::sizedUrl(const QUrl &url, Size size)
{
    if (size == Size::Original || !url.host().endsWith(u"coverartarchive.org"_s))
        return url;
    QString path = url.path();
    static const QStringList images{u"/front"_s, u"/back"_s};
    for (const QString &image : images) {
        if (path.endsWith(image)) {
            path += u"-%1"_s.arg(int(size));
            QUrl result = url;
            result.setPath(path);
            return result;
        }
    }
    return url;
}

int CoverArtFetcher::fetch(const QUrl &url, Size size)
{
    const int id = m_nextId++;
    if (!url.isValid() || !m_network) {
        const QString error = !m_network ? u"No network access manager"_s : u"Invalid cover URL"_s;
        QTimer::singleShot(0, this, [this, id, error] {
            Q_EMIT finished(id, Metadata::CoverArt(), error);
        });
        return id;
    }

    QNetworkReply *reply = m_network->get(request(sizedUrl(url, size)));
    m_replies.insert(id, reply);

    connect(reply, &QNetworkReply::downloadProgress, this, [this, id, reply](qint64 received, qint64 total) {
        if (received > m_maximumBytes || total > m_maximumBytes) {
            disconnect(reply, nullptr, this, nullptr);
            m_replies.remove(id);
            reply->abort();
            reply->deleteLater();
            Q_EMIT finished(id, Metadata::CoverArt(), u"Cover image exceeds %1 MiB"_s.arg(m_maximumBytes / (1024 * 1024)));
            return;
        }
        Q_EMIT progress(id, received, total);
    });

    connect(reply, &QNetworkReply::finished, this, [this, id, reply] {
        reply->deleteLater();
        if (m_replies.value(id) != reply)
            return; // canceled
        m_replies.remove(id);

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 404) {
            Q_EMIT finished(id, Metadata::CoverArt(), QString());
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT finished(id, Metadata::CoverArt(), u"Cover download failed: %1"_s.arg(reply->errorString()));
            return;
        }
        const auto cover = Metadata::CoverArt::fromData(reply->readAll(), reply->url().toString());
        if (!cover) {
            Q_EMIT finished(id, Metadata::CoverArt(), u"The downloaded file is not an image"_s);
            return;
        }
        Q_EMIT finished(id, *cover, QString());
    });
    return id;
}

int CoverArtFetcher::fetchListing(const QUrl &url)
{
    const int id = m_nextId++;
    if (!url.isValid() || !m_network) {
        const QString error = !m_network ? u"No network access manager"_s : u"Invalid listing URL"_s;
        QTimer::singleShot(0, this, [this, id, error] {
            Q_EMIT listingFinished(id, CoverArtListing(), error);
        });
        return id;
    }

    QNetworkRequest listingRequest = request(url);
    listingRequest.setRawHeader("Accept", "application/json");
    QNetworkReply *reply = m_network->get(listingRequest);
    m_replies.insert(id, reply);
    connect(reply, &QNetworkReply::finished, this, [this, id, reply] {
        reply->deleteLater();
        if (m_replies.value(id) != reply)
            return; // canceled
        m_replies.remove(id);

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 404) {
            Q_EMIT listingFinished(id, CoverArtListing(), QString()); // no images
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT listingFinished(id, CoverArtListing(), u"Cover listing failed: %1"_s.arg(reply->errorString()));
            return;
        }
        QString error;
        const std::optional<CoverArtListing> listing = parseCoverArtListing(reply->readAll(), &error);
        Q_EMIT listingFinished(id, listing.value_or(CoverArtListing()), listing ? QString() : u"Cover listing unreadable: %1"_s.arg(error));
    });
    return id;
}

void CoverArtFetcher::cancel(int requestId)
{
    if (QPointer<QNetworkReply> reply = m_replies.take(requestId)) {
        disconnect(reply, nullptr, this, nullptr);
        reply->abort(); // closes the connection now, not only when the reply is deleted
        reply->deleteLater();
    }
}

bool CoverArtFetcher::isRunning(int requestId) const
{
    return m_replies.contains(requestId);
}

QNetworkRequest CoverArtFetcher::request(const QUrl &url) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setMaximumRedirectsAllowed(10);
    request.setTransferTimeout(60000);
    return request;
}

}
