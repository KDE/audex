/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "metadata.h"

#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QSaveFile>

using namespace Qt::StringLiterals;

namespace Audex::Metadata
{

// ---- CoverArt ---------------------------------------------------------------

QString CoverArt::suffix() const
{
    const QMimeType mime = QMimeDatabase().mimeTypeForName(mimeType);
    if (mimeType == u"image/jpeg")
        return u"jpg"_s;
    return mime.isValid() ? mime.preferredSuffix() : QString();
}

std::optional<CoverArt> CoverArt::fromData(const QByteArray &data, const QString &source)
{
    if (data.isEmpty())
        return std::nullopt;
    const QMimeType mime = QMimeDatabase().mimeTypeForData(data);
    if (!mime.name().startsWith(u"image/"))
        return std::nullopt;
    CoverArt cover;
    cover.data = data;
    cover.mimeType = mime.name();
    cover.source = source;
    return cover;
}

std::optional<CoverArt> CoverArt::fromFile(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return std::nullopt;
    }
    auto cover = fromData(file.readAll(), path);
    if (!cover && error)
        *error = u"%1 is not an image"_s.arg(path);
    return cover;
}

bool CoverArt::saveToFile(const QString &path, QString *error) const
{
    if (isNull()) {
        if (error)
            *error = u"No cover image"_s;
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

// ---- Album: tracks ---------------------------------------------------------

bool Album::hasTrack(int number) const
{
    return m_tracks.contains(number);
}

const Track &Album::track(int number) const
{
    static const Track empty;
    const auto it = m_tracks.constFind(number);
    return it == m_tracks.cend() ? empty : it.value();
}

Track &Album::track(int number)
{
    Q_ASSERT(number >= 0 && number <= 99);
    return m_tracks[number];
}

void Album::setTrack(int number, const Track &track)
{
    Q_ASSERT(number >= 0 && number <= 99);
    const auto it = m_tracks.constFind(number);
    if (it != m_tracks.cend() && it.value() == track)
        return;
    m_tracks.insert(number, track);
    m_structureModified = true;
}

bool Album::removeTrack(int number)
{
    const auto it = m_tracks.find(number);
    if (it == m_tracks.end())
        return false;
    const bool hadContent = !it.value().isEmpty();
    m_tracks.erase(it);
    if (hadContent)
        m_structureModified = true;
    return hadContent;
}

QList<int> Album::trackNumbers() const
{
    return m_tracks.keys();
}

QList<int> Album::regularTrackNumbers() const
{
    QList<int> result;
    for (auto it = m_tracks.cbegin(); it != m_tracks.cend(); ++it)
        if (it.key() >= 1)
            result.append(it.key());
    return result;
}

// ---- Album: cover ---------------------------------------------------------

bool Album::setCover(const CoverArt &cover)
{
    if (m_cover == cover)
        return false;
    m_cover = cover;
    m_structureModified = true;
    return true;
}

bool Album::removeCover()
{
    return setCover(CoverArt());
}

// ---- Album: state -----------------------------------------------------------

bool Album::isEmpty() const
{
    if (!Fields::isEmpty() || !m_cover.isNull())
        return false;
    for (const Track &t : m_tracks)
        if (!t.isEmpty())
            return false;
    return true;
}

bool Album::isModified() const
{
    if (m_structureModified || Fields::isModified())
        return true;
    for (const Track &t : m_tracks)
        if (t.isModified())
            return true;
    return false;
}

void Album::confirm()
{
    Fields::confirm();
    for (Track &t : m_tracks)
        t.confirm();
    m_structureModified = false;
}

void Album::reset()
{
    Fields::reset();
    m_tracks.clear();
    m_cover = CoverArt();
    m_structureModified = false;
}

bool Album::merge(const Album &other, MergePolicy policy)
{
    bool changed = Fields::merge(other, policy);
    for (auto it = other.m_tracks.cbegin(); it != other.m_tracks.cend(); ++it) {
        if (it.value().isEmpty())
            continue;
        const bool existed = hasTrack(it.key());
        const bool trackChanged = track(it.key()).merge(it.value(), policy);
        if (trackChanged && !existed)
            m_structureModified = true;
        changed |= trackChanged;
    }
    if (!other.m_cover.isNull() && (policy == MergePolicy::Overwrite || m_cover.isNull()))
        changed |= setCover(other.m_cover);
    return changed;
}

bool Album::operator==(const Album &other) const
{
    if (!Fields::operator==(other) || !(m_cover == other.m_cover))
        return false;
    // empty tracks do not count
    auto nonEmpty = [](const QMap<int, Track> &tracks) {
        QMap<int, Track> r;
        for (auto it = tracks.cbegin(); it != tracks.cend(); ++it)
            if (!it.value().isEmpty())
                r.insert(it.key(), it.value());
        return r;
    };
    const QMap<int, Track> a = nonEmpty(m_tracks);
    const QMap<int, Track> b = nonEmpty(other.m_tracks);
    if (a.keys() != b.keys())
        return false;
    for (auto it = a.cbegin(); it != a.cend(); ++it)
        if (it.value() != b.value(it.key()))
            return false;
    return true;
}

// ---- Album: helpers -------------------------------------------------------

namespace
{
// "Artist feat. Guest" counts as "Artist"
QString primaryArtist(const QString &artist)
{
    static const QRegularExpression featuring(uR"(\s+(?:feat\.?|ft\.|featuring)\s+.*$)"_s, QRegularExpression::CaseInsensitiveOption);
    return QString(artist).remove(featuring).trimmed();
}
}

bool Album::guessVariousArtists() const
{
    QString first;
    for (int n : regularTrackNumbers()) {
        const QString artist = primaryArtist(track(n).text(Field::Artist));
        if (artist.isEmpty())
            continue;
        if (first.isEmpty())
            first = artist;
        else if (artist.compare(first, Qt::CaseInsensitive) != 0)
            return true;
    }
    return false;
}

std::optional<DiscHint> Album::guessMultiDisc() const
{
    static const QRegularExpression rx(uR"(^(.*?)[\s\-:,]*[\(\[]?\s*\b(?:cd|disc|disk)\s*(\d{1,2})(?:\s*(?:of|/)\s*(\d{1,2}))?\s*[\)\]]?\s*$)"_s,
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = rx.match(text(Field::Album));
    if (!m.hasMatch())
        return std::nullopt;
    DiscHint hint;
    hint.album = m.captured(1).trimmed();
    hint.number = m.captured(2).toInt();
    hint.count = m.captured(3).toInt();
    if (hint.number < 1 || hint.album.isEmpty() || (hint.count > 0 && hint.number > hint.count))
        return std::nullopt;
    return hint;
}

bool Album::swapTrackArtistsAndTitles()
{
    bool changed = false;
    for (int n : regularTrackNumbers())
        changed |= m_tracks[n].swap(Field::Artist, Field::Title);
    return changed;
}

bool Album::swapArtistAndAlbum()
{
    return swap(Field::Artist, Field::Album);
}

bool Album::splitTrackTitles(const QString &divider)
{
    bool changed = false;
    for (int n : regularTrackNumbers())
        changed |= m_tracks[n].split(Field::Title, divider, Field::Artist, Field::Title);
    return changed;
}

bool Album::capitalizeTracks()
{
    bool changed = false;
    for (int n : regularTrackNumbers()) {
        changed |= m_tracks[n].capitalize(Field::Artist);
        changed |= m_tracks[n].capitalize(Field::Title);
    }
    return changed;
}

bool Album::capitalizeAlbum()
{
    bool changed = capitalize(Field::Artist);
    changed |= capitalize(Field::Album);
    return changed;
}

bool Album::setTrackArtistsFromAlbum(bool onlyEmpty)
{
    const QString artist = text(Field::Artist);
    bool changed = false;
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        if (onlyEmpty && it.value().contains(Field::Artist))
            continue;
        changed |= it.value().setText(Field::Artist, artist);
    }
    return changed;
}

QDebug operator<<(QDebug debug, const Album &album)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Album(" << static_cast<const Fields &>(album);
    for (auto it = album.tracks().cbegin(); it != album.tracks().cend(); ++it)
        debug << ", track " << it.key() << ": " << static_cast<const Fields &>(it.value());
    if (!album.cover().isNull())
        debug << ", cover " << album.cover().mimeType << ' ' << album.cover().data.size() << " bytes";
    debug << ')';
    return debug;
}

}
