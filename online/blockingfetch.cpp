/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "blockingfetch.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

using namespace Qt::StringLiterals;

namespace Audex::Online
{

QString userAgent()
{
    return u"Audex/%1 ( https://apps.kde.org/audex/ )"_s.arg(QCoreApplication::applicationVersion());
}

QByteArray fetchBlocking(const QUrl &url,
                         QString *error,
                         bool *notFound,
                         int timeoutMs,
                         const std::function<bool()> &isCanceled,
                         const QList<std::pair<QByteArray, QByteArray>> &headers)
{
    if (notFound)
        *notFound = false;
    if (error)
        error->clear();

    QNetworkAccessManager network;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent());
    for (const auto &[name, value] : headers)
        request.setRawHeader(name, value);

    QNetworkReply *reply = network.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    QTimer poll;
    if (isCanceled) {
        QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
            if (isCanceled())
                loop.quit();
        });
        poll.start(100);
    }
    loop.exec();

    QByteArray data;
    if (!reply->isFinished()) {
        // close the connection properly instead of tearing it down together
        // with the access manager
        reply->abort();
        if (error && isCanceled && isCanceled())
            *error = QCoreApplication::translate("Audex::Online", "canceled");
        else if (error)
            *error = QCoreApplication::translate("Audex::Online", "timeout after %1 s").arg(timeoutMs / 1000);
    } else if (reply->error() == QNetworkReply::ContentNotFoundError) {
        if (notFound)
            *notFound = true;
    } else if (reply->error() != QNetworkReply::NoError) {
        if (error)
            *error = reply->errorString();
    } else {
        data = reply->readAll();
    }
    reply->deleteLater();
    return data;
}

}
