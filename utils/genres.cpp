/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "genres.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace Audex::Genres
{

QStringList presets()
{
    QStringList genres;

    const QStringList files = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, u"genres.json"_s, QStandardPaths::LocateFile);
    if (files.isEmpty()) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            qWarning().noquote() << "genres.json was not found in" << QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).join(u", "_s)
                                 << "- is Audex installed, and is its data directory in XDG_DATA_DIRS? The genre list stays empty.";
        }
    }

    for (const QString &fileName : std::as_const(files)) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isArray()) {
            continue;
        }

        const QJsonArray array = doc.array();
        for (const QJsonValue &value : array) {
            const QString genre = value.toString().trimmed();
            if (!genre.isEmpty() && !genres.contains(genre, Qt::CaseInsensitive)) {
                genres << genre;
            }
        }
    }

    genres.sort(Qt::CaseInsensitive);
    return genres;
}

} // namespace Audex::Genres
