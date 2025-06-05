/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "metadata.h"

namespace Audex
{

namespace Metadata
{

const Track &Metadata::track(const int tracknumber) const
{
    if (tracknumber == 0)
        return htoa;
    return subset(tracknumber - 1);
}

Track &Metadata::track(const int tracknumber)
{
    if (tracknumber == 0)
        return htoa;
    return subset(tracknumber - 1);
}

const Track &Metadata::firstTrack() const
{
    return firstSubset();
}

Track &Metadata::firstTrack()
{
    return firstSubset();
}

const Track &Metadata::lastTrack() const
{
    return lastSubset();
}

Track &Metadata::lastTrack()
{
    return lastSubset();
}

// return tracknumber
int Metadata::appendTrack(const Track &track)
{
    return appendSubset(track) + 1;
}

int Metadata::trackCount() const
{
    return subsetCount();
}

void Metadata::setCDDBCategory(const QString &category)
{
    QStringList validCategories;
    validCategories << QStringLiteral(u"blues") << QStringLiteral(u"classical") << QStringLiteral(u"country") << QStringLiteral(u"data")
                    << QStringLiteral(u"folk") << QStringLiteral(u"jazz") << QStringLiteral(u"misc") << QStringLiteral(u"newage") << QStringLiteral(u"reggae")
                    << QStringLiteral(u"rock") << QStringLiteral(u"soundtrack");
    if (!validCategories.contains(category.toLower()))
        set(CDDBCategory, QStringLiteral(u"misc"));
    else
        set(CDDBCategory, category.toLower());
}

const QImage Metadata::cover() const
{
    return get(Cover).value<QImage>();
}

void Metadata::setCover(const QImage &cover)
{
    set(Cover, cover);
}

bool Metadata::loadCoverFromFile(const QString &filename)
{
    QImage cover;
    if (cover.load(filename)) {
        setCover(cover);
        return true;
    }
    return false;
}

bool Metadata::saveCoverToFile(const QString &filename)
{
    if (cover().save(filename))
        return true;
    return false;
}

bool Metadata::guessVarious() const
{
    QString artist;
    for (int t = 1; t < trackCount(); ++t) {
        if (t > 1 && track(t).get(Artist).toString().toLower() != artist.toLower())
            return true;
        artist = track(t).get(Artist).toString();
    }
    return false;
}

void Metadata::setVarious(bool various)
{
    set(VariousArtists, various);
}

bool Metadata::isVarious()
{
    return get(VariousArtists).toBool();
}

int Metadata::guessMultiDisc(QString &newAlbum) const
{
    QString t = get(Title).toString();
    QRegularExpression rx1(QStringLiteral(u"[\\(|\\[]* *([c|C][d|D]|[d|D][i|I][s|S][k|c|K|C]) *[0-9]* *[\\)|\\]]* *$"));
    QRegularExpressionMatch match1 = rx1.match(t);
    if (match1.hasMatch()) {
        QString frac = match1.captured(0);
        QRegularExpression rx2(QStringLiteral(u"(\\d+)"));
        QRegularExpressionMatch match2 = rx2.match(frac);

        bool ok;
        int cdnum = match2.captured(0).toInt(&ok);
        if (ok) {
            if (cdnum < 0)
                return -1;
            if (cdnum == 0)
                cdnum = 1;
            newAlbum = t.left(match1.capturedStart(0)).trimmed();
            return cdnum;
        }
    }
    return -1;
}

void Metadata::setMultiDisc(const bool multi)
{
    set(MultiDisc, multi);
}

bool Metadata::isMultiDisc() const
{
    return get(MultiDisc, false).toBool();
}

void Metadata::setDiscNum(const int n)
{
    set(DiscNumber, n);
}

int Metadata::discNum() const
{
    if (isMultiDisc())
        return get(DiscNumber).toInt();
    return 0;
}

void Metadata::swapArtistAndTitleOfTracks()
{
    for (int t = 1; t <= trackCount(); ++t) {
        if (track(t).contains(Title) && track(t).contains(Artist)) {
            QVariant tmp = track(t).get(Artist);
            track(t).set(Artist, track(t).get(Title));
            track(t).set(Title, tmp);
        }
    }
}

void Metadata::swapArtistAndAlbum()
{
    if (contains(Album) && contains(Artist)) {
        QVariant tmp = get(Album);
        set(Album, get(Artist));
        set(Artist, tmp);
    }
}

void Metadata::splitTitleOfTracks(const QString &divider)
{
    for (int t = 1; t < trackCount(); ++t) {
        int splitPos = track(t).get(Title).toString().indexOf(divider);
        if (splitPos >= 0) {
            // split
            QString title = track(t).get(Title).toString().mid(splitPos + divider.length());
            QString artist = track(t).get(Title).toString().left(splitPos);
            track(t).set(Artist, artist);
            track(t).set(Title, title);
        }
    }
}

void Metadata::capitalizeTracks()
{
    for (int t = 1; t < trackCount(); ++t) {
        if (track(t).contains(Artist))
            track(t).set(Artist, p_capitalize(track(t).get(Artist).toString()));
        if (track(t).contains(Title))
            track(t).set(Title, p_capitalize(track(t).get(Title).toString()));
    }
}

void Metadata::capitalizeHeader()
{
    if (contains(Artist))
        set(Artist, p_capitalize(get(Artist).toString()));
    if (contains(Album))
        set(Album, p_capitalize(get(Album).toString()));
}

void Metadata::setTitleArtistsFromHeader()
{
    for (int t = 1; t <= trackCount(); ++t)
        track(t).set(Artist, get(Artist));
}

const QString Metadata::capitalize(const QString &s)
{
    QStringList stringlist = s.split(u' ', Qt::SkipEmptyParts);
    for (int i = 0; i < stringlist.count(); ++i) {
        QString string = stringlist[i].toLower();
        int j = 0;
        while (((string[j] == u'(') || (string[j] == u'[') || (string[j] == u'{')) && (j < string.length()))
            j++;
        string[j] = string[j].toUpper();
        stringlist[i] = string;
    }
    return stringlist.join(u' ');
}

QDebug operator<<(QDebug debug, const Metadata &metadata)
{
    QDebugStateSaver saver(debug);

    for (int i = 0; i < static_cast<int>(Invalid); ++i) {
        Type type = static_cast<Type>(i);
        if (metadata.contains(type))
            debug.nospace() << "Type=" << type << ", Value=" << metadata.get(type) << Qt::endl;
    }

    debug.nospace() << "Found " << metadata.trackCount() << " tracks:" << Qt::endl;

    for (int t = 1; t <= metadata.trackCount(); ++t) {
        debug.nospace() << "Track: " << t;
        for (int j = 0; j < static_cast<int>(Invalid); ++j) {
            Type type = static_cast<Type>(j);
            if (metadata.track(t).contains(type))
                debug.nospace() << ", Type=" << type << ", Value=" << metadata.track(t).get(type);
        }
        debug.nospace() << '\n';
    }
    return debug;
}

}

}
