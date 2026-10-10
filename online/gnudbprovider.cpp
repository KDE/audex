/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "gnudbprovider.h"

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Audex
{

namespace
{

// The name gnudb knows this program by. It has to match the registration at
// info@gnudb.org; generic names get blocked.
const auto DefaultClient = u"Audex"_s;

QString userAgent()
{
    return u"Audex/%1 ( https://apps.kde.org/audex/ )"_s.arg(QCoreApplication::applicationVersion());
}

// Exact matches first: with a limited number of reads those are the ones worth
// reading. Order within a group stays as the server sent it.
void sortMatches(QList<Gnudb::QueryMatch> &matches)
{
    std::stable_partition(matches.begin(), matches.end(), [](const Gnudb::QueryMatch &m) {
        return m.exact;
    });
}

}

GnudbProvider::GnudbProvider(QNetworkAccessManager *network, QObject *parent)
    : MetadataProvider(parent)
    , m_network(network)
    , m_client(DefaultClient)
    , m_version(QCoreApplication::applicationVersion())
    , m_server(QString::fromLatin1(Gnudb::DefaultServer))
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &GnudbProvider::schedule);
}

QString GnudbProvider::id() const
{
    return u"gnudb"_s;
}

QString GnudbProvider::displayName() const
{
    return u"gnudb"_s;
}

bool GnudbProvider::isAvailable() const
{
    return !hello().isEmpty();
}

void GnudbProvider::setEmail(const QString &email)
{
    m_email = email.trimmed();
}

QString GnudbProvider::email() const
{
    return m_email;
}

void GnudbProvider::setClient(const QString &client, const QString &version)
{
    m_client = client;
    m_version = version;
}

void GnudbProvider::setServer(const QUrl &server)
{
    m_server = server;
}

void GnudbProvider::setMinimumInterval(int ms)
{
    m_interval = std::max(0, ms);
}

void GnudbProvider::setMaximumReads(int count)
{
    m_maximumReads = std::max(1, count);
}

QString GnudbProvider::hello() const
{
    return Gnudb::helloString(m_email, m_client, m_version);
}

void GnudbProvider::lookup(int requestId, const CDInfo &disc)
{
    Request r;
    r.id = requestId;
    r.disc = disc;
    m_queue.enqueue(r);
    if (!m_timer.isActive())
        m_timer.start(0); // never emit from within lookup()
}

void GnudbProvider::cancel(int requestId)
{
    for (auto it = m_queue.begin(); it != m_queue.end();) {
        if (it->id == requestId)
            it = m_queue.erase(it);
        else
            ++it;
    }
    if (QPointer<QNetworkReply> reply = m_active.take(requestId)) {
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
    }
}

void GnudbProvider::fail(int requestId, const QString &error)
{
    QTimer::singleShot(0, this, [this, requestId, error] {
        Q_EMIT finished(requestId, {}, error);
    });
}

void GnudbProvider::schedule()
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

void GnudbProvider::send(Request request)
{
    const QString helloString = hello();
    if (helloString.isEmpty()) {
        fail(request.id, u"gnudb needs an email address; see the settings"_s);
        return;
    }
    if (!m_network) {
        fail(request.id, u"No network access manager"_s);
        return;
    }

    QUrl url;
    if (request.nextMatch < 0) {
        url = Gnudb::queryUrl(request.disc, helloString, m_server);
        if (!url.isValid()) {
            fail(request.id, QString()); // no TOC: nothing to ask for
            return;
        }
    } else {
        const Gnudb::QueryMatch &match = request.matches.at(request.nextMatch);
        url = Gnudb::readUrl(match.category, match.discId, helloString, m_server);
        if (!url.isValid()) {
            ++request.nextMatch; // unusable entry, try the next one
            continueOrFinish(request);
            return;
        }
    }

    QNetworkRequest nr(url);
    nr.setHeader(QNetworkRequest::UserAgentHeader, userAgent());
    nr.setRawHeader("Accept", "text/plain");
    nr.setTransferTimeout(30000);

    m_lastRequest.restart();
    ++request.attempts;
    QNetworkReply *reply = m_network->get(nr);
    m_active.insert(request.id, reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, request] {
        handleReply(reply, request);
    });
}

void GnudbProvider::handleReply(QNetworkReply *reply, Request request)
{
    reply->deleteLater();
    if (m_active.value(request.id) != reply)
        return; // canceled
    m_active.remove(request.id);

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if ((status == 503 || status == 429) && request.attempts < 3) {
        // the server asks for a break, the same request follows later
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
        const QString error = u"gnudb: %1"_s.arg(reply->errorString());
        if (request.nextMatch < 0) {
            fail(request.id, error); // without the query there is nothing to read
            return;
        }
        if (request.error.isEmpty())
            request.error = error;
        ++request.nextMatch; // one entry is missing, the others may work
        continueOrFinish(request);
        return;
    }

    const QByteArray data = reply->readAll();
    if (request.nextMatch < 0)
        handleQuery(data, request);
    else
        handleRead(data, request);
    continueOrFinish(request);
}

void GnudbProvider::handleQuery(const QByteArray &data, Request &request)
{
    const Gnudb::QueryResult result = Gnudb::parseQueryResponse(data);
    if (!result.error.isEmpty() && request.error.isEmpty())
        request.error = result.error;
    request.matches = result.matches;
    sortMatches(request.matches);
    if (request.matches.size() > m_maximumReads)
        request.matches.resize(m_maximumReads);
    request.nextMatch = 0;
    request.attempts = 0;
}

void GnudbProvider::handleRead(const QByteArray &data, Request &request)
{
    const Gnudb::QueryMatch &match = request.matches.at(request.nextMatch);
    const Gnudb::ReadResult result = Gnudb::parseReadResponse(data, request.disc, match.category, match.discId, match.exact);
    if (!result.error.isEmpty() && request.error.isEmpty())
        request.error = result.error;
    if (result.found)
        request.candidates.append(result.candidate);
    ++request.nextMatch;
    request.attempts = 0;
}

void GnudbProvider::continueOrFinish(Request request)
{
    if (request.nextMatch >= 0 && request.nextMatch < request.matches.size()) {
        m_queue.enqueue(request);
        if (!m_timer.isActive())
            m_timer.start(0); // schedule() keeps the interval
        return;
    }

    MetadataCandidates candidates = request.candidates;
    sortCandidates(candidates);
    // an error is only worth reporting if nothing was found at all
    const QString error = candidates.isEmpty() ? request.error : QString();
    Q_EMIT finished(request.id, candidates, error);
}

}
