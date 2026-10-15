/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "responsecache.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrlQuery>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Audex
{

ResponseCache::ResponseCache(const QString &directory)
    : m_directory(directory)
{
}

QString ResponseCache::defaultDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + u"/audex/metadata"_s;
}

void ResponseCache::setMaximumAge(int days)
{
    m_maximumAge = std::max(NoLimit, days);
}

int ResponseCache::maximumAge() const
{
    return m_maximumAge;
}

void ResponseCache::setMaximumSize(qint64 bytes)
{
    m_maximumSize = std::max<qint64>(NoLimit, bytes);
}

QString ResponseCache::key(const QUrl &url)
{
    QUrl normalized = url.adjusted(QUrl::NormalizePathSegments | QUrl::StripTrailingSlash);
    QUrlQuery query(normalized);
    query.removeAllQueryItems(u"hello"_s); // gnudb: the email address of the user
    normalized.setQuery(query);
    return QString::fromLatin1(QCryptographicHash::hash(normalized.toEncoded(), QCryptographicHash::Sha256).toHex());
}

QString ResponseCache::path(const QString &service, const QUrl &url) const
{
    return QDir(m_directory).filePath(service + u'/' + key(url));
}

QString ResponseCache::namedPath(const QString &service, const QString &name) const
{
    const QByteArray hash = QCryptographicHash::hash(name.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QDir(m_directory).filePath(service + u'/' + QString::fromLatin1(hash));
}

std::optional<QByteArray> ResponseCache::find(const QString &service, const QUrl &url) const
{
    if (service.isEmpty() || !url.isValid())
        return std::nullopt;
    return read(path(service, url));
}

void ResponseCache::insert(const QString &service, const QUrl &url, const QByteArray &data)
{
    if (service.isEmpty() || !url.isValid())
        return;
    write(path(service, url), data);
}

std::optional<QByteArray> ResponseCache::findNamed(const QString &service, const QString &name) const
{
    if (service.isEmpty() || name.isEmpty())
        return std::nullopt;
    return read(namedPath(service, name));
}

void ResponseCache::insertNamed(const QString &service, const QString &name, const QByteArray &data)
{
    if (service.isEmpty() || name.isEmpty())
        return;
    write(namedPath(service, name), data);
}

void ResponseCache::removeNamed(const QString &service, const QString &name)
{
    if (!service.isEmpty() && !name.isEmpty())
        QFile::remove(namedPath(service, name));
}

std::optional<QByteArray> ResponseCache::read(const QString &file) const
{
    if (m_maximumAge == 0)
        return std::nullopt;
    const QFileInfo info(file);
    if (!info.exists())
        return std::nullopt;
    if (m_maximumAge != NoLimit && info.lastModified().daysTo(QDateTime::currentDateTime()) >= m_maximumAge) {
        QFile::remove(file);
        return std::nullopt;
    }
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly))
        return std::nullopt;
    return f.readAll();
}

void ResponseCache::write(const QString &file, const QByteArray &data)
{
    if (m_maximumAge == 0 || data.isEmpty())
        return;
    if (!QDir().mkpath(QFileInfo(file).path()))
        return;
    QSaveFile f(file);
    if (f.open(QIODevice::WriteOnly) && f.write(data) == data.size())
        f.commit();
    trim();
}

qint64 ResponseCache::size() const
{
    qint64 bytes = 0;
    QDirIterator it(m_directory, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        bytes += it.fileInfo().size();
    }
    return bytes;
}

void ResponseCache::clear()
{
    QDir(m_directory).removeRecursively();
}

void ResponseCache::trim() const
{
    if (m_maximumSize == NoLimit)
        return;
    QList<QFileInfo> files;
    qint64 bytes = 0;
    QDirIterator it(m_directory, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        files << it.fileInfo();
        bytes += it.fileInfo().size();
    }
    if (bytes <= m_maximumSize)
        return;
    std::sort(files.begin(), files.end(), [](const QFileInfo &a, const QFileInfo &b) {
        return a.lastModified() < b.lastModified();
    });
    for (const QFileInfo &file : std::as_const(files)) {
        if (bytes <= m_maximumSize)
            break;
        if (QFile::remove(file.absoluteFilePath()))
            bytes -= file.size();
    }
}

}
