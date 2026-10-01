/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdinfo.h"

#include "core/discid.h"

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Audex
{

using Metadata::Field;

// ---- MetadataCandidate ---------------------------------------------------------

QString MetadataCandidate::description() const
{
    QString s = album.text(Field::Artist);
    const QString title = album.text(Field::Album);
    if (!title.isEmpty())
        s += (s.isEmpty() ? QString() : u" – "_s) + title;
    if (s.isEmpty())
        s = u"(unnamed)"_s;

    QStringList details;
    if (album.contains(Field::Year))
        details << album.text(Field::Year);
    if (album.contains(Field::Country))
        details << album.text(Field::Country);
    if (album.number(Field::DiscCount) > 1)
        details << u"disc %1/%2"_s.arg(album.number(Field::DiscNumber)).arg(album.number(Field::DiscCount));
    if (album.contains(Field::DiscSubtitle))
        details << album.text(Field::DiscSubtitle);
    if (!details.isEmpty())
        s += u" ("_s + details.join(u", "_s) + u')';
    return s;
}

void sortCandidates(MetadataCandidates &candidates)
{
    std::stable_sort(candidates.begin(), candidates.end(), [](const MetadataCandidate &a, const MetadataCandidate &b) {
        return a.score > b.score;
    });
}

// ---- CDInfo --------------------------------------------------------------------

CDInfo::CDInfo(const Cdda::Toc &toc, const QString &discUdi)
    : m_toc(toc)
    , m_discUdi(discUdi)
{
    if (!m_toc.isEmpty()) {
        m_cddbDiscId = Cdda::cddbDiscIdString(m_toc);
        m_musicBrainzDiscId = Cdda::musicBrainzDiscId(m_toc);
        m_musicBrainzToc = Cdda::musicBrainzTocString(m_toc);
    }
}

void CDInfo::clear()
{
    *this = CDInfo();
}

QString CDInfo::mediumDescription() const
{
    QString text;
    switch (m_medium) {
    case Medium::Pressed:
        return u"pressed CD"_s;
    case Medium::CdR:
        text = u"CD-R"_s;
        break;
    case Medium::CdRw:
        text = u"CD-RW"_s;
        break;
    case Medium::Unknown:
        return u"unknown"_s;
    }
    return m_mediumDetails.isEmpty() ? text : u"%1 (%2)"_s.arg(text, m_mediumDetails);
}

bool CDInfo::hasHtoa() const
{
    return m_toc.htoaSectorCount() > 0;
}

int CDInfo::htoaSectorCount() const
{
    return m_toc.htoaSectorCount();
}

QList<DiscEntry> CDInfo::entries() const
{
    QList<DiscEntry> result;
    if (hasHtoa()) {
        DiscEntry e;
        e.number = 0;
        e.hidden = true;
        e.firstLba = 0;
        e.lastLba = htoaSectorCount() - 1;
        result.append(e);
    }
    for (const Cdda::Track &t : m_toc.tracks) {
        DiscEntry e;
        e.number = t.number;
        e.audio = t.audio;
        e.session = t.session;
        e.firstLba = t.firstLba;
        e.lastLba = t.lastLba;
        result.append(e);
    }
    return result;
}

std::optional<DiscEntry> CDInfo::entry(int number) const
{
    for (const DiscEntry &e : entries())
        if (e.number == number)
            return e;
    return std::nullopt;
}

bool CDInfo::hasEntry(int number) const
{
    if (number == 0)
        return hasHtoa();
    return m_toc.track(number) != nullptr;
}

bool CDInfo::isAudioTrack(int number) const
{
    if (number == 0)
        return hasHtoa();
    const Cdda::Track *t = m_toc.track(number);
    return t && t->audio;
}

QList<int> CDInfo::audioTrackNumbers(bool includeHtoa) const
{
    QList<int> result;
    if (includeHtoa && hasHtoa())
        result.append(0);
    result += m_toc.audioTrackNumbers();
    return result;
}

QList<int> CDInfo::preEmphasisTracks() const
{
    QList<int> result;
    for (const Cdda::Track &t : m_toc.tracks)
        if (t.audio && t.preEmphasis)
            result.append(t.number);
    return result;
}

int CDInfo::trackNumberForAudioPosition(int position) const
{
    const QList<int> audio = m_toc.audioTrackNumbers();
    if (position < 1 || position > audio.size())
        return 0;
    return audio.at(position - 1);
}

int CDInfo::audioPositionOfTrack(int number) const
{
    const qsizetype i = m_toc.audioTrackNumbers().indexOf(number);
    return i < 0 ? 0 : int(i) + 1;
}

int CDInfo::displayTrackNumber(int number) const
{
    if (number == 0)
        return 0;
    return number + m_metadata.number(Field::TrackNumberOffset);
}

void CDInfo::setMetadata(const Metadata::Album &album)
{
    m_metadata = album;
    pruneMetadata();
}

const Metadata::Track &CDInfo::trackMetadata(int number) const
{
    return m_metadata.track(number);
}

Metadata::Track &CDInfo::trackMetadata(int number)
{
    Q_ASSERT(hasEntry(number));
    return m_metadata.track(number);
}

void CDInfo::pruneMetadata()
{
    for (int number : m_metadata.trackNumbers())
        if (!hasEntry(number))
            m_metadata.removeTrack(number);
}

void CDInfo::applyCandidate(const MetadataCandidate &candidate, Metadata::MergePolicy policy)
{
    if (policy == Metadata::MergePolicy::Overwrite) {
        Metadata::Album album = candidate.album;
        // keep what belongs to the disc, not to the database entry
        if (m_metadata.contains(Field::MCN) && !album.contains(Field::MCN))
            album.setText(Field::MCN, m_metadata.text(Field::MCN));
        for (int number : m_metadata.trackNumbers()) {
            const QString isrc = m_metadata.track(number).text(Field::ISRC);
            if (!isrc.isEmpty() && !album.track(number).contains(Field::ISRC) && hasEntry(number))
                album.track(number).setText(Field::ISRC, isrc);
        }
        // assignment would lose the "modified" information for the editor
        m_metadata.clear();
        for (int number : m_metadata.trackNumbers())
            m_metadata.track(number).clear();
        m_metadata.merge(album, Metadata::MergePolicy::Overwrite);
        if (album.cover().isNull())
            m_metadata.removeCover();
    } else {
        m_metadata.merge(candidate.album, Metadata::MergePolicy::KeepExisting);
    }
    pruneMetadata();
    completeMetadata();
}

void CDInfo::completeMetadata()
{
    if (!m_cddbDiscId.isEmpty())
        m_metadata.setText(Field::CddbDiscId, m_cddbDiscId);
    if (!m_musicBrainzDiscId.isEmpty())
        m_metadata.setText(Field::MusicBrainzDiscId, m_musicBrainzDiscId);

    // "false" is stored as absent, so only a positive guess is applied
    if (!m_metadata.flag(Field::VariousArtists) && m_metadata.guessVariousArtists())
        m_metadata.setFlag(Field::VariousArtists, true);

    if (hasHtoa() && !m_metadata.track(0).contains(Field::Artist) && m_metadata.contains(Field::Artist) && !m_metadata.flag(Field::VariousArtists))
        m_metadata.track(0).setText(Field::Artist, m_metadata.text(Field::Artist));
}

bool CDInfo::operator==(const CDInfo &other) const
{
    return m_toc == other.m_toc && m_discUdi == other.m_discUdi && m_metadata == other.m_metadata;
}

}
