/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "blockingfetch.h"

#include <QCoreApplication>
#include <QElapsedTimer>
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
                         const QList<std::pair<QByteArray, QByteArray>> &headers,
                         const std::function<bool(const QUrl &)> &allowRedirect)
{
    if (notFound)
        *notFound = false;
    if (error)
        error->clear();

    QNetworkAccessManager network;
    QElapsedTimer elapsed;
    elapsed.start();
    QUrl current = url;
    int redirects = 0;
    while (true) {
        const int remaining = timeoutMs - int(elapsed.elapsed());
        if (remaining <= 0) {
            if (error)
                *error = QCoreApplication::translate("Audex::Online", "timeout after %1 s").arg(timeoutMs / 1000);
            return {};
        }
        QNetworkRequest request(current);
        request.setHeader(QNetworkRequest::UserAgentHeader, userAgent());
        if (allowRedirect)
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
        for (const auto &[name, value] : headers)
            request.setRawHeader(name, value);

        QNetworkReply *reply = network.get(request);
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(remaining);
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
        bool follow = false;
        if (!reply->isFinished()) {
            reply->abort();
            if (error && isCanceled && isCanceled())
                *error = QCoreApplication::translate("Audex::Online", "canceled");
            else if (error)
                *error = QCoreApplication::translate("Audex::Online", "timeout after %1 s").arg(timeoutMs / 1000);
        } else if (allowRedirect && reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
            const QUrl next = current.resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
            if (redirects < 5 && allowRedirect(next)) {
                current = next;
                ++redirects;
                follow = true;
            } else if (error) {
                *error = QCoreApplication::translate("Audex::Online", "redirect not allowed");
            }
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
        if (!follow)
            return data;
    }
}

}
