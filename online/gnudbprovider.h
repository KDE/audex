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

#include "metadata/gnudb.h"
#include "metadata/lookup.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace Audex
{

// gnudb (CDDB) lookup over HTTP. Two steps per disc, as the protocol demands:
// "cddb query" delivers the matching entries, "cddb read" the entry itself.
// gnudb requires a contact address and a descriptive client name in every
// request; without an address the provider is not available.
class GnudbProvider : public MetadataProvider
{
    Q_OBJECT

public:
    explicit GnudbProvider(QNetworkAccessManager *network, QObject *parent = nullptr);

    QString id() const override;
    QString displayName() const override;
    bool isAvailable() const override; // only with a usable email address

    void setEmail(const QString &email);
    QString email() const;
    void setClient(const QString &client, const QString &version);
    void setServer(const QUrl &server);
    void setMinimumInterval(int ms); // default 1000 ms
    void setMaximumReads(int count); // entries read per disc, default 5

    void lookup(int requestId, const CDInfo &disc) override;
    void cancel(int requestId) override;

private:
    struct Request {
        int id = 0;
        CDInfo disc;
        QList<Gnudb::QueryMatch> matches; // filled by the query
        int nextMatch = -1; // -1: the query is still due
        MetadataCandidates candidates;
        QString error;
        int attempts = 0;
    };

    void schedule();
    void send(Request request);
    void handleReply(QNetworkReply *reply, Request request);
    void handleQuery(const QByteArray &data, Request &request);
    void handleRead(const QByteArray &data, Request &request);
    void continueOrFinish(Request request);
    void fail(int requestId, const QString &error);
    QString hello() const;

    QPointer<QNetworkAccessManager> m_network;
    QString m_email;
    QString m_client;
    QString m_version;
    QUrl m_server;
    int m_interval = 1000;
    int m_maximumReads = 5;

    QQueue<Request> m_queue;
    QHash<int, QPointer<QNetworkReply>> m_active;
    QTimer m_timer;
    QElapsedTimer m_lastRequest;
};

}
