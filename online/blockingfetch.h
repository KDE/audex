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
#include <QUrl>

#include <functional>

namespace Audex::Online
{

QString userAgent();

// Synchronous GET for worker threads (runs its own event loop). Sets notFound
// when the server answered 404 and error on network or protocol failures.
QByteArray fetchBlocking(const QUrl &url,
                         QString *error,
                         bool *notFound = nullptr,
                         int timeoutMs = 30000,
                         const std::function<bool()> &isCanceled = {},
                         const QList<std::pair<QByteArray, QByteArray>> &headers = {});

}
