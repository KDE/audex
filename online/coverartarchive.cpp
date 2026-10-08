/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "coverartarchive.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

namespace Audex
{

namespace
{

QUrl secure(const QString &address)
{
    QUrl url(address);
    const QString host = url.host();
    if (url.scheme() == u"http" && (host.endsWith(u"archive.org") || host.endsWith(u"musicbrainz.org")))
        url.setScheme(u"https"_s);
    return url;
}

}

QUrl CoverArtImage::url(int size) const
{
    if (size > 0) {
        for (auto it = thumbnails.lowerBound(size); it != thumbnails.cend(); ++it)
            if (it.value().isValid())
                return it.value();
    }
    return image;
}

std::optional<CoverArtListing> parseCoverArtListing(const QByteArray &json, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (!doc.isObject() || !doc.object().value("images"_L1).isArray()) {
        if (error)
            *error = parseError.error != QJsonParseError::NoError ? parseError.errorString() : u"not a Cover Art Archive listing"_s;
        return std::nullopt;
    }

    CoverArtListing listing;
    listing.release = secure(doc.object().value("release"_L1).toString());
    const QJsonArray images = doc.object().value("images"_L1).toArray();
    for (const QJsonValue &value : images) {
        const QJsonObject o = value.toObject();
        CoverArtImage image;
        image.image = secure(o.value("image"_L1).toString());
        if (!image.image.isValid())
            continue;
        // "small" and "large" are the old names of 250 and 500
        const QJsonObject thumbnails = o.value("thumbnails"_L1).toObject();
        for (auto it = thumbnails.constBegin(); it != thumbnails.constEnd(); ++it) {
            bool numeric = false;
            int size = it.key().toInt(&numeric);
            if (!numeric)
                size = it.key() == u"small" ? 250 : it.key() == u"large" ? 500 : 0;
            if (size > 0 && !image.thumbnails.contains(size))
                image.thumbnails.insert(size, secure(it.value().toString()));
        }
        for (const QJsonValue &type : o.value("types"_L1).toArray())
            image.types << type.toString();
        image.comment = o.value("comment"_L1).toString();
        image.front = o.value("front"_L1).toBool();
        listing.images.append(image);
    }
    return listing;
}

}
