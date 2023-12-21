/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CACHEDIMAGE_H
#define CACHEDIMAGE_H

#include <QAbstractTableModel>
#include <QBuffer>
#include <QCryptographicHash>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QString>

#include <KLocalizedString>

#include "utils/error.h"

class CachedImage
{
public:
    CachedImage();
    explicit CachedImage(const QByteArray &data);
    explicit CachedImage(const QString &filename);
    CachedImage(const CachedImage &other);
    CachedImage &operator=(const CachedImage &other);
    bool operator==(const CachedImage &other) const;
    bool operator!=(const CachedImage &other) const;
    ~CachedImage();

    Error lastError() const;

    bool isEmpty() const;

    const QByteArray formatRaw() const;
    int dataSize(const QByteArray &format = "", const QSize &size = QSize());
    const QByteArray coverRaw() const;
    const QSize size() const;
    const QImage coverImage() const;
    const QString checksum() const;

    const QString supportedMimeTypeList();
    const QList<QByteArray> supportedFormats();

    bool load(const QByteArray &data);
    bool load(const QString &filename);

    void clear();

    bool save(const QString &filename, const QSize &size = QSize());

private:
    QByteArray _format;
    QByteArray _data;
    QSize _size;

    bool compare_format(const QByteArray &f1, const QByteArray &f2) const;

    bool _save(QIODevice *device, const QByteArray &format = "", const QSize &size = QSize());

    QString mime_cache;

    Error _error;
};

#endif
