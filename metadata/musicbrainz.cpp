/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "musicbrainz.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

using namespace Qt::StringLiterals;

namespace Audex::MusicBrainz
{

using Metadata::Field;

QUrl discIdLookupUrl(const CDInfo &disc, const QUrl &server)
{
    if (disc.musicBrainzDiscId().isEmpty())
        return QUrl();
    QUrl url = server;
    url.setPath(u"/ws/2/discid/"_s + disc.musicBrainzDiscId());
    QString toc = disc.musicBrainzTocString();
    toc.replace(u' ', u'+');
    url.setQuery(u"toc=%1&cdstubs=no&inc=%2&fmt=json"_s.arg(toc, QString::fromLatin1(IncludeParameter)));
    return url;
}

QString artistCreditString(const QJsonArray &credit)
{
    QString result;
    for (const QJsonValue &v : credit) {
        const QJsonObject c = v.toObject();
        QString name = c.value("name"_L1).toString();
        if (name.isEmpty())
            name = c.value("artist"_L1).toObject().value("name"_L1).toString();
        result += name + c.value("joinphrase"_L1).toString();
    }
    return result.trimmed();
}

namespace
{

QString firstArtistId(const QJsonArray &credit)
{
    return credit.isEmpty() ? QString() : credit.first().toObject().value("artist"_L1).toObject().value("id"_L1).toString();
}

bool mediumHasDisc(const QJsonObject &medium, const QString &discId)
{
    const QJsonArray discs = medium.value("discs"_L1).toArray();
    for (const QJsonValue &d : discs)
        if (d.toObject().value("id"_L1).toString() == discId)
            return true;
    return false;
}

void fillTrack(Metadata::Track &track, const QJsonObject &t)
{
    const QJsonObject recording = t.value("recording"_L1).toObject();

    QString title = t.value("title"_L1).toString();
    if (title.isEmpty())
        title = recording.value("title"_L1).toString();
    track.setText(Field::Title, title);

    QJsonArray credit = t.value("artist-credit"_L1).toArray();
    if (credit.isEmpty())
        credit = recording.value("artist-credit"_L1).toArray();
    track.setText(Field::Artist, artistCreditString(credit));
    track.setText(Field::MusicBrainzArtistId, firstArtistId(credit));

    track.setText(Field::MusicBrainzTrackId, t.value("id"_L1).toString());
    track.setText(Field::MusicBrainzRecordingId, recording.value("id"_L1).toString());
    const QJsonArray isrcs = recording.value("isrcs"_L1).toArray();
    if (!isrcs.isEmpty())
        track.setText(Field::ISRC, isrcs.first().toString());
}

}

ParseResult parseDiscIdResponse(const QByteArray &json, const CDInfo &disc)
{
    ParseResult result;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result.error = u"Invalid MusicBrainz response: %1"_s.arg(parseError.errorString());
        return result;
    }
    const QJsonObject root = doc.object();
    if (root.contains("error"_L1))
        return result; // "Not Found": no candidates, not an error

    const bool exactLookup = root.value("id"_L1).toString() == disc.musicBrainzDiscId();
    const int audioTracks = int(disc.audioTrackNumbers().size());

    const QJsonArray releases = root.value("releases"_L1).toArray();
    for (const QJsonValue &rv : releases) {
        const QJsonObject release = rv.toObject();
        const QJsonArray media = release.value("media"_L1).toArray();

        // If a medium carries our disc id, only that medium is a candidate
        // (a two disc set may contain two media with the same track count).
        bool releaseHasDisc = false;
        for (const QJsonValue &mv : media)
            releaseHasDisc |= mediumHasDisc(mv.toObject(), disc.musicBrainzDiscId());

        for (const QJsonValue &mv : media) {
            const QJsonObject medium = mv.toObject();
            const int trackCount = medium.value("track-count"_L1).toInt(int(medium.value("tracks"_L1).toArray().size()));

            int score = 0;
            if (releaseHasDisc)
                score = mediumHasDisc(medium, disc.musicBrainzDiscId()) ? 100 : 0;
            else if (trackCount == audioTracks)
                score = exactLookup ? 90 : 70;
            if (score == 0)
                continue;

            MetadataCandidate c;
            c.provider = u"musicbrainz"_s;
            c.providerName = u"MusicBrainz"_s;
            c.reference = release.value("id"_L1).toString();
            c.score = score;

            Metadata::Album &a = c.album;
            const QJsonArray credit = release.value("artist-credit"_L1).toArray();
            a.setText(Field::Album, release.value("title"_L1).toString());
            a.setText(Field::Artist, artistCreditString(credit));
            a.setText(Field::MusicBrainzArtistId, firstArtistId(credit));
            a.setFlag(Field::VariousArtists, firstArtistId(credit) == QString::fromLatin1(VariousArtistsId));
            a.setText(Field::Year, release.value("date"_L1).toString().left(4));
            a.setText(Field::Country, release.value("country"_L1).toString());
            a.setText(Field::Barcode, release.value("barcode"_L1).toString());
            a.setText(Field::MusicBrainzReleaseId, c.reference);
            a.setText(Field::MusicBrainzReleaseGroupId, release.value("release-group"_L1).toObject().value("id"_L1).toString());
            a.setText(Field::MusicBrainzDiscId, disc.musicBrainzDiscId());
            if (media.size() > 1) {
                a.setFlag(Field::MultiDisc, true);
                a.setNumber(Field::DiscNumber, medium.value("position"_L1).toInt());
                a.setNumber(Field::DiscCount, int(media.size()));
            }
            a.setText(Field::DiscSubtitle, medium.value("title"_L1).toString());

            const QJsonArray tracks = medium.value("tracks"_L1).toArray();
            for (const QJsonValue &tv : tracks) {
                const QJsonObject t = tv.toObject();
                const int number = disc.trackNumberForAudioPosition(t.value("position"_L1).toInt());
                if (number == 0)
                    continue;
                Metadata::Track track;
                fillTrack(track, t);
                a.setTrack(number, track);
            }

            const QJsonObject pregap = medium.value("pregap"_L1).toObject();
            if (!pregap.isEmpty() && disc.hasHtoa()) {
                Metadata::Track track;
                fillTrack(track, pregap);
                a.setTrack(0, track);
            }

            // the release's own front cover, else the one chosen for its release group
            const QJsonObject caa = release.value("cover-art-archive"_L1).toObject();
            if (caa.value("front"_L1).toBool() && !c.reference.isEmpty())
                c.covers.append({QUrl(u"https://coverartarchive.org/release/%1/front"_s.arg(c.reference)),
                                 QUrl(u"https://musicbrainz.org/release/%1/cover-art"_s.arg(c.reference)),
                                 false});
            const QString groupId = a.text(Field::MusicBrainzReleaseGroupId);
            if (!groupId.isEmpty())
                c.covers.append({QUrl(u"https://coverartarchive.org/release-group/%1/front"_s.arg(groupId)),
                                 QUrl(u"https://musicbrainz.org/release-group/%1/cover-art"_s.arg(groupId)),
                                 true});

            a.confirm();
            result.candidates.append(c);
        }
    }

    sortCandidates(result.candidates);
    return result;
}

}
