/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "gnudb.h"

#include <QMap>
#include <QStringConverter>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Audex::Gnudb
{

using Metadata::Field;

namespace
{

// An exact disc id match is good, but the CDDB disc id is only 32 bits wide;
// MusicBrainz (90/100) stays ahead of it, its fuzzy TOC match (70) does not.
constexpr int ExactScore = 80;
constexpr int InexactScore = 50;

// " / " separates artist and title, nothing else does
const auto TitleSeparator = u" / "_s;

QString decode(const QByteArray &raw)
{
    QStringDecoder utf8(QStringConverter::Utf8);
    const QString text = utf8.decode(raw);
    if (utf8.hasError())
        return QString::fromLatin1(raw); // an older entry, not level 6
    return text;
}

// Keeps the value usable inside a "+" separated CDDB command
QString sanitizeField(const QString &text)
{
    QString result;
    for (const QChar c : text) {
        if (c.isSpace() || c == u'+' || c == u'&' || c == u'=' || c == u'%' || c == u'#' || c == u'?' || c == u'/')
            continue;
        result += c;
    }
    return result;
}

bool looksLikeUuid(const QString &text)
{
    if (text.size() != 36)
        return false;
    for (qsizetype i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != u'-')
                return false;
        } else if (!c.isDigit() && (c.toLower() < u'a' || c.toLower() > u'f')) {
            return false;
        }
    }
    return true;
}

}

// ---- requests ------------------------------------------------------------

bool isUsableEmail(const QString &email)
{
    const qsizetype at = email.indexOf(u'@');
    if (at <= 0 || at == email.size() - 1)
        return false;
    const QString host = email.mid(at + 1);
    if (!host.contains(u'.') || host.startsWith(u'.') || host.endsWith(u'.'))
        return false;
    return !sanitizeField(email.left(at)).isEmpty() && sanitizeField(host) == host;
}

QString helloString(const QString &email, const QString &client, const QString &version)
{
    if (!isUsableEmail(email))
        return QString();
    const qsizetype at = email.indexOf(u'@');
    const QString name = sanitizeField(email.left(at));
    const QString host = sanitizeField(email.mid(at + 1));
    const QString app = sanitizeField(client);
    const QString ver = sanitizeField(version);
    if (name.isEmpty() || host.isEmpty() || app.isEmpty() || ver.isEmpty())
        return QString();
    return u"%1 %2 %3 %4"_s.arg(name, host, app, ver);
}

QString queryCommand(const CDInfo &disc)
{
    const Cdda::Toc &toc = disc.toc();
    if (toc.isEmpty() || disc.cddbDiscId().isEmpty())
        return QString();

    QStringList parts;
    parts << u"cddb"_s << u"query"_s << disc.cddbDiscId() << QString::number(toc.tracks.size());
    for (const Cdda::Track &t : toc.tracks)
        parts << QString::number(t.firstLba + Cdda::PregapSectors);
    parts << QString::number((toc.leadOutLba + Cdda::PregapSectors) / Cdda::SectorsPerSecond);
    return parts.join(u' ');
}

QUrl commandUrl(const QString &command, const QString &hello, const QUrl &server)
{
    if (command.isEmpty() || hello.isEmpty())
        return QUrl();

    // Spaces are written as "+", everything else percent encoded. The query is
    // assembled by hand: the CDDB parameters are not ordinary key/value pairs.
    const auto encode = [](const QString &text) {
        QString result;
        const QList<QByteArray> words = text.toUtf8().split(' ');
        for (const QByteArray &word : words) {
            if (!result.isEmpty())
                result += u'+';
            result += QString::fromLatin1(QUrl::toPercentEncoding(QString::fromUtf8(word)));
        }
        return result;
    };

    QUrl url = server;
    url.setPath(QString::fromLatin1(CgiPath));
    url.setQuery(u"cmd=%1&hello=%2&proto=%3"_s.arg(encode(command), encode(hello), QString::number(ProtocolLevel)));
    return url;
}

QUrl queryUrl(const CDInfo &disc, const QString &hello, const QUrl &server)
{
    return commandUrl(queryCommand(disc), hello, server);
}

QUrl readUrl(const QString &category, const QString &discId, const QString &hello, const QUrl &server)
{
    if (category.isEmpty() || discId.isEmpty())
        return QUrl();
    return commandUrl(u"cddb read %1 %2"_s.arg(category, discId), hello, server);
}

// ---- responses -----------------------------------------------------------

Response parseResponse(const QByteArray &raw)
{
    Response result;

    QString text = decode(raw);
    text.replace(u"\r\n"_s, u"\n"_s);
    text.replace(u'\r', u'\n');
    QStringList lines = text.split(u'\n');
    while (!lines.isEmpty() && lines.constFirst().trimmed().isEmpty())
        lines.removeFirst();
    if (lines.isEmpty())
        return result;

    const QString statusLine = lines.takeFirst();
    const qsizetype space = statusLine.indexOf(u' ');
    const QString codeText = space < 0 ? statusLine.trimmed() : statusLine.left(space);
    bool ok = false;
    const int code = codeText.toInt(&ok);
    if (!ok || code < 100 || code > 599)
        return result; // not a CDDB answer
    result.code = code;
    result.status = space < 0 ? QString() : statusLine.mid(space + 1).trimmed();

    for (const QString &line : std::as_const(lines)) {
        if (line.trimmed() == u"."_s)
            break;
        result.lines.append(line);
    }
    // a trailing empty line of the HTTP body is not data
    while (!result.lines.isEmpty() && result.lines.constLast().trimmed().isEmpty())
        result.lines.removeLast();
    return result;
}

QueryResult parseQueryResponse(const QByteArray &raw)
{
    QueryResult result;
    const Response response = parseResponse(raw);

    const auto appendMatch = [&result](const QString &line, bool exact) {
        const QStringList parts = line.simplified().split(u' ');
        if (parts.size() < 2)
            return;
        QueryMatch m;
        m.category = parts.at(0);
        m.discId = parts.at(1);
        m.title = parts.mid(2).join(u' ');
        m.exact = exact;
        if (!m.category.isEmpty() && !m.discId.isEmpty())
            result.matches.append(m);
    };

    switch (response.code) {
    case 200: // one exact match, on the status line itself
        appendMatch(response.status, true);
        break;
    case 210: // exact matches as a list
    case 211: // inexact matches
        for (const QString &line : response.lines)
            appendMatch(line, response.code == 210);
        break;
    case 202: // nothing found - not an error
        break;
    case 0:
        result.error = u"gnudb: unexpected answer"_s;
        break;
    default:
        result.error = u"gnudb: %1 %2"_s.arg(QString::number(response.code), response.status);
        break;
    }
    return result;
}

QString unescape(const QString &text)
{
    QString result;
    result.reserve(text.size());
    for (qsizetype i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c != u'\\' || i + 1 >= text.size()) {
            result += c;
            continue;
        }
        const QChar next = text.at(++i);
        if (next == u'n')
            result += u'\n';
        else if (next == u't')
            result += u'\t';
        else if (next == u'\\')
            result += u'\\';
        else
            result += next; // unknown sequence: keep the character
    }
    return result;
}

TitleParts splitTitle(const QString &text)
{
    TitleParts parts;
    const qsizetype separator = text.indexOf(TitleSeparator);
    if (separator < 0) {
        parts.title = text.trimmed();
        return parts;
    }
    parts.artist = text.left(separator).trimmed();
    parts.title = text.mid(separator + TitleSeparator.size()).trimmed();
    return parts;
}

ReadResult parseXmcd(const QStringList &lines, const CDInfo &disc, const QString &category, const QString &discId, bool exact)
{
    ReadResult result;

    QMap<int, QString> titles; // TTITLEn, 0 based
    QMap<int, QString> trackComments; // EXTTn
    QString discTitle;
    QString year;
    QString genre;
    QString comment; // EXTD
    QStringList coverIds; // "# Artid:"
    QStringList coverUrls; // "# Cover:"
    int offsetCount = 0;

    for (const QString &line : lines) {
        if (line.startsWith(u'#')) {
            const QString body = line.mid(1).trimmed();
            if (!body.isEmpty() && std::all_of(body.cbegin(), body.cend(), [](QChar c) {
                    return c.isDigit();
                })) {
                ++offsetCount; // one entry of the track frame offset list
            } else if (body.startsWith(u"Artid:"_s, Qt::CaseInsensitive)) {
                const QString id = body.mid(6).trimmed();
                if (looksLikeUuid(id) && !coverIds.contains(id))
                    coverIds.append(id);
            } else if (body.startsWith(u"Cover:"_s, Qt::CaseInsensitive)) {
                const QString url = body.mid(6).trimmed();
                if (!url.isEmpty() && !coverUrls.contains(url))
                    coverUrls.append(url);
            }
            continue;
        }

        const qsizetype equals = line.indexOf(u'=');
        if (equals <= 0)
            continue;
        const QString key = line.left(equals).trimmed().toUpper();
        const QString data = line.mid(equals + 1);

        // repeated keywords are concatenated, that is how long texts are stored
        const auto index = [&key](qsizetype prefix) {
            bool ok = false;
            const int n = key.mid(prefix).toInt(&ok);
            return (ok && n >= 0 && n < 100) ? n : -1;
        };

        if (key == u"DTITLE"_s)
            discTitle += data;
        else if (key == u"DYEAR"_s)
            year += data;
        else if (key == u"DGENRE"_s)
            genre += data;
        else if (key == u"EXTD"_s)
            comment += data;
        else if (key.startsWith(u"TTITLE"_s)) {
            const int n = index(6);
            if (n >= 0)
                titles[n] += data;
        } else if (key.startsWith(u"EXTT"_s)) {
            const int n = index(4);
            if (n >= 0)
                trackComments[n] += data;
        }
    }

    if (titles.isEmpty() && discTitle.isEmpty()) {
        result.error = u"gnudb: the entry contains no titles"_s;
        return result;
    }

    // Does the entry describe a disc of this size? CDDB counts data tracks as
    // tracks, but not every client does, so both readings are accepted.
    QList<int> tocNumbers;
    for (const Cdda::Track &t : disc.toc().tracks)
        tocNumbers.append(t.number);
    const QList<int> audioNumbers = disc.audioTrackNumbers();
    const int trackCount = titles.isEmpty() ? 0 : titles.lastKey() + 1;

    QList<int> mapping;
    if (trackCount == tocNumbers.size())
        mapping = tocNumbers;
    else if (trackCount == audioNumbers.size())
        mapping = audioNumbers;
    else
        return result; // a different disc, not an error

    if (offsetCount > 0 && offsetCount != tocNumbers.size() && offsetCount != audioNumbers.size())
        return result;

    MetadataCandidate &candidate = result.candidate;
    candidate.provider = u"gnudb"_s;
    candidate.providerName = u"gnudb"_s;
    candidate.reference = u"%1/%2"_s.arg(category, discId);
    candidate.score = exact ? ExactScore : InexactScore;

    Metadata::Album &album = candidate.album;
    const TitleParts titleParts = splitTitle(unescape(discTitle));
    album.setText(Field::Album, titleParts.title);
    // "artist / title" missing means both are the same (xmcd format)
    album.setText(Field::Artist, titleParts.artist.isEmpty() ? titleParts.title : titleParts.artist);
    const QString albumArtist = album.text(Field::Artist);
    const bool various = albumArtist.compare(u"various"_s, Qt::CaseInsensitive) == 0 //
        || albumArtist.startsWith(u"various artists"_s, Qt::CaseInsensitive);
    album.setFlag(Field::VariousArtists, various);

    const QString yearText = unescape(year).trimmed();
    if (yearText != u"0"_s)
        album.setText(Field::Year, yearText);
    album.setText(Field::Genre, unescape(genre).trimmed());
    album.setText(Field::Comment, unescape(comment).trimmed());
    album.setText(Field::CddbCategory, category);
    album.setText(Field::CddbDiscId, discId);

    for (auto it = titles.cbegin(); it != titles.cend(); ++it) {
        const int number = mapping.value(it.key(), 0);
        if (number == 0 || !disc.isAudioTrack(number))
            continue; // a data track carries no title

        Metadata::Track track;
        const QString text = unescape(it.value());
        // On a sampler the track title carries the artist as well
        const TitleParts parts = various ? splitTitle(text) : TitleParts{QString(), text.trimmed()};
        track.setText(Field::Artist, parts.artist);
        track.setText(Field::Title, parts.title);
        track.setText(Field::Comment, unescape(trackComments.value(it.key())).trimmed());
        album.setTrack(number, track);
    }

    // gnudb stores the cover art ids of the matching MusicBrainz releases; the
    // "/front" address is the same one the MusicBrainz candidates use, so the
    // configured cover size works for these as well.
    for (const QString &id : std::as_const(coverIds))
        candidate.covers.append({QUrl(u"https://coverartarchive.org/release/%1/front"_s.arg(id)), //
                                 QUrl(u"https://musicbrainz.org/release/%1/cover-art"_s.arg(id)),
                                 false});
    if (coverIds.isEmpty()) {
        for (const QString &url : std::as_const(coverUrls))
            candidate.covers.append({QUrl(url), QUrl(), false});
    }

    album.confirm();
    result.found = true;
    return result;
}

ReadResult parseReadResponse(const QByteArray &raw, const CDInfo &disc, const QString &category, const QString &discId, bool exact)
{
    const Response response = parseResponse(raw);

    // "210 <category> <discid> CD database entry follows": only used if the
    // caller does not know what it asked for
    QString entryCategory = category;
    QString entryDiscId = discId;
    const QStringList status = response.status.simplified().split(u' ');
    if (status.size() >= 2) {
        if (entryCategory.isEmpty())
            entryCategory = status.at(0);
        if (entryDiscId.isEmpty())
            entryDiscId = status.at(1);
    }

    ReadResult result;
    switch (response.code) {
    case 210: // the entry follows
    case 200:
        result = parseXmcd(response.lines, disc, entryCategory, entryDiscId, exact);
        break;
    case 401: // no such entry - not an error
        break;
    case 0:
        result.error = u"gnudb: unexpected answer"_s;
        break;
    default:
        result.error = u"gnudb: %1 %2"_s.arg(QString::number(response.code), response.status);
        break;
    }
    return result;
}

}
