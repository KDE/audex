/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QPointer>
#include <QQueue>
#include <QTimer>
#include <QUrl>

#include "metadata/lookup.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace Audex
{

// MusicBrainz lookup via the JSON web service.
// Respects the rate limit (one request per second per client) and sends a
// meaningful User-Agent as required by MusicBrainz.
class MusicBrainzProvider : public MetadataProvider
{
    Q_OBJECT

public:
    explicit MusicBrainzProvider(QNetworkAccessManager *network, QObject *parent = nullptr);

    QString id() const override;
    QString displayName() const override;

    void setUserAgent(const QString &userAgent);
    void setServer(const QUrl &server);
    void setMinimumInterval(int ms); // default 1100 ms

    void lookup(int requestId, const CDInfo &disc) override;
    void cancel(int requestId) override;

    static QString defaultUserAgent();

private:
    struct Request {
        int id = 0;
        CDInfo disc;
        int attempts = 0;
    };

    void schedule();
    void send(Request request);
    void handleReply(QNetworkReply *reply, const Request &request);

    QPointer<QNetworkAccessManager> m_network;
    QString m_userAgent;
    QUrl m_server;
    int m_interval = 1100;

    QQueue<Request> m_queue;
    QHash<int, QPointer<QNetworkReply>> m_active;
    QTimer m_timer;
    QElapsedTimer m_lastRequest;
};

}
