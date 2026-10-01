/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "musicbrainzprovider.h"

#include "metadata/musicbrainz.h"

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

using namespace Qt::StringLiterals;

namespace Audex
{

MusicBrainzProvider::MusicBrainzProvider(QNetworkAccessManager *network, QObject *parent)
    : MetadataProvider(parent)
    , m_network(network)
    , m_userAgent(defaultUserAgent())
    , m_server(QString::fromLatin1(MusicBrainz::DefaultServer))
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &MusicBrainzProvider::schedule);
}

QString MusicBrainzProvider::id() const
{
    return u"musicbrainz"_s;
}

QString MusicBrainzProvider::displayName() const
{
    return u"MusicBrainz"_s;
}

void MusicBrainzProvider::setUserAgent(const QString &userAgent)
{
    m_userAgent = userAgent;
}

void MusicBrainzProvider::setServer(const QUrl &server)
{
    m_server = server;
}

void MusicBrainzProvider::setMinimumInterval(int ms)
{
    m_interval = std::max(0, ms);
}

QString MusicBrainzProvider::defaultUserAgent()
{
    return u"Audex/%1 ( https://apps.kde.org/audex/ )"_s.arg(QCoreApplication::applicationVersion());
}

void MusicBrainzProvider::lookup(int requestId, const CDInfo &disc)
{
    Request r;
    r.id = requestId;
    r.disc = disc;
    m_queue.enqueue(r);
    if (!m_timer.isActive())
        m_timer.start(0); // never emit from within lookup()
}

void MusicBrainzProvider::cancel(int requestId)
{
    for (auto it = m_queue.begin(); it != m_queue.end();) {
        if (it->id == requestId)
            it = m_queue.erase(it);
        else
            ++it;
    }
    if (QPointer<QNetworkReply> reply = m_active.take(requestId)) {
        disconnect(reply, nullptr, this, nullptr);
        reply->abort(); // closes the connection now, not only when the reply is deleted
        reply->deleteLater();
    }
}

void MusicBrainzProvider::schedule()
{
    if (m_queue.isEmpty())
        return;
    if (m_lastRequest.isValid() && m_lastRequest.elapsed() < m_interval) {
        m_timer.start(int(m_interval - m_lastRequest.elapsed()));
        return;
    }
    send(m_queue.dequeue());
    if (!m_queue.isEmpty())
        m_timer.start(m_interval);
}

void MusicBrainzProvider::send(Request request)
{
    const QUrl url = MusicBrainz::discIdLookupUrl(request.disc, m_server);
    if (!url.isValid() || !m_network) {
        const QString error = m_network ? QString() : u"No network access manager"_s;
        const int id = request.id;
        QTimer::singleShot(0, this, [this, id, error] {
            Q_EMIT finished(id, {}, error);
        });
        return;
    }

    QNetworkRequest nr(url);
    nr.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    nr.setRawHeader("Accept", "application/json");
    nr.setTransferTimeout(30000);

    m_lastRequest.restart();
    ++request.attempts;
    QNetworkReply *reply = m_network->get(nr);
    m_active.insert(request.id, reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, request] {
        handleReply(reply, request);
    });
}

void MusicBrainzProvider::handleReply(QNetworkReply *reply, const Request &request)
{
    reply->deleteLater();
    if (m_active.value(request.id) != reply)
        return; // canceled
    m_active.remove(request.id);

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status == 404) {
        Q_EMIT finished(request.id, {}, QString());
        return;
    }

    if ((status == 503 || status == 429) && request.attempts < 3) {
        // rate limited: try again later
        int delay = std::max(m_interval, 2000);
        bool ok = false;
        const int retryAfter = reply->rawHeader("Retry-After").toInt(&ok);
        if (ok && retryAfter > 0)
            delay = std::min(retryAfter * 1000, 30000);
        m_queue.prepend(request);
        m_timer.start(delay);
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        Q_EMIT finished(request.id, {}, u"MusicBrainz: %1"_s.arg(reply->errorString()));
        return;
    }

    const MusicBrainz::ParseResult parsed = MusicBrainz::parseDiscIdResponse(reply->readAll(), request.disc);
    Q_EMIT finished(request.id, parsed.candidates, parsed.error);
}

}
