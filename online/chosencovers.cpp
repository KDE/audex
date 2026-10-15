/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "chosencovers.h"

#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

namespace Audex::ChosenCovers
{

// One file: a line of JSON (where the image comes from), then the image as delivered

std::optional<Metadata::CoverArt> find(const ResponseCache &cache, const QString &discId)
{
    const std::optional<QByteArray> entry = cache.findNamed(QString::fromLatin1(Service), discId);
    if (!entry)
        return std::nullopt;
    const qsizetype newline = entry->indexOf('\n');
    if (newline < 0)
        return std::nullopt;
    const QJsonObject info = QJsonDocument::fromJson(entry->left(newline)).object();
    std::optional<Metadata::CoverArt> cover = Metadata::CoverArt::fromData(entry->mid(newline + 1), info.value("source"_L1).toString());
    if (!cover)
        return std::nullopt;
    cover->origin = info.value("origin"_L1).toString();
    cover->originPage = QUrl(info.value("originPage"_L1).toString());
    return cover;
}

void keep(ResponseCache &cache, const QString &discId, const Metadata::CoverArt &cover)
{
    if (cover.isNull())
        return;
    QJsonObject info;
    info.insert("source"_L1, cover.source);
    info.insert("origin"_L1, cover.origin);
    if (cover.originPage.isValid())
        info.insert("originPage"_L1, cover.originPage.toString());
    cache.insertNamed(QString::fromLatin1(Service), discId, QJsonDocument(info).toJson(QJsonDocument::Compact) + '\n' + cover.data);
}

void forget(ResponseCache &cache, const QString &discId)
{
    cache.removeNamed(QString::fromLatin1(Service), discId);
}

}
