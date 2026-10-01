/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "discid.h"

#include <QCryptographicHash>
#include <QRegularExpression>
#include <QStringList>

using namespace Qt::StringLiterals;

namespace Audex::Cdda
{

namespace
{

inline int absolute(int lba)
{
    return lba + PregapSectors;
}

int digitSum(int n)
{
    int s = 0;
    while (n > 0) {
        s += n % 10;
        n /= 10;
    }
    return s;
}

}

// ---- CDDB ----------------------------------------------------------------------

quint32 cddbDiscId(const Toc &toc)
{
    if (toc.tracks.isEmpty())
        return 0;
    quint32 n = 0;
    for (const Track &t : toc.tracks)
        n += digitSum(absolute(t.firstLba) / SectorsPerSecond);
    const quint32 total = quint32(absolute(toc.leadOutLba) / SectorsPerSecond - absolute(toc.tracks.first().firstLba) / SectorsPerSecond);
    return ((n % 255) << 24) | ((total & 0xFFFF) << 8) | (quint32(toc.tracks.size()) & 0xFF);
}

QString cddbDiscIdString(const Toc &toc)
{
    return u"%1"_s.arg(cddbDiscId(toc), 8, 16, QLatin1Char('0'));
}

// ---- MusicBrainz ---------------------------------------------------------------

std::optional<MusicBrainzToc> musicBrainzToc(const Toc &toc)
{
    qsizetype lastAudio = -1;
    for (qsizetype i = toc.tracks.size() - 1; i >= 0; --i) {
        if (toc.tracks.at(i).audio) {
            lastAudio = i;
            break;
        }
    }
    if (lastAudio < 0)
        return std::nullopt;

    MusicBrainzToc r;
    r.firstTrack = toc.tracks.first().number;
    r.lastTrack = toc.tracks.at(lastAudio).number;
    if (lastAudio + 1 < toc.tracks.size())
        r.leadOut = absolute(toc.tracks.at(lastAudio + 1).firstLba) - EnhancedCdSessionGap;
    else
        r.leadOut = absolute(toc.leadOutLba);

    for (qsizetype i = 0; i <= lastAudio; ++i)
        r.offsets.append(absolute(toc.tracks.at(i).firstLba));

    // Track numbers must be contiguous for the 99 slot layout
    if (r.offsets.size() != r.lastTrack - r.firstTrack + 1)
        return std::nullopt;
    return r;
}

QString musicBrainzDiscId(const MusicBrainzToc &toc)
{
    QByteArray text;
    text.reserve(2 + 2 + 8 * 100);
    text += QByteArray::number(toc.firstTrack, 16).toUpper().rightJustified(2, '0');
    text += QByteArray::number(toc.lastTrack, 16).toUpper().rightJustified(2, '0');
    text += QByteArray::number(toc.leadOut, 16).toUpper().rightJustified(8, '0');
    for (int number = 1; number <= 99; ++number) {
        int offset = 0;
        if (number >= toc.firstTrack && number <= toc.lastTrack)
            offset = toc.offsets.value(number - toc.firstTrack);
        text += QByteArray::number(offset, 16).toUpper().rightJustified(8, '0');
    }

    QByteArray b64 = QCryptographicHash::hash(text, QCryptographicHash::Sha1).toBase64();
    b64.replace('+', '.').replace('/', '_').replace('=', '-');
    return QString::fromLatin1(b64);
}

QString musicBrainzDiscId(const Toc &toc)
{
    const auto mb = musicBrainzToc(toc);
    return mb ? musicBrainzDiscId(*mb) : QString();
}

QString musicBrainzTocString(const MusicBrainzToc &toc, QChar separator)
{
    QStringList parts;
    parts << QString::number(toc.firstTrack) << QString::number(toc.lastTrack) << QString::number(toc.leadOut);
    for (int o : toc.offsets)
        parts << QString::number(o);
    return parts.join(separator);
}

QString musicBrainzTocString(const Toc &toc, QChar separator)
{
    const auto mb = musicBrainzToc(toc);
    return mb ? musicBrainzTocString(*mb, separator) : QString();
}

std::optional<Toc> tocFromMusicBrainzTocString(const QString &text)
{
    static const QRegularExpression separators(u"[\\s+]+"_s);
    const QStringList parts = text.trimmed().split(separators, Qt::SkipEmptyParts);
    if (parts.size() < 4)
        return std::nullopt;

    QList<int> values;
    for (const QString &p : parts) {
        bool ok = false;
        const int v = p.toInt(&ok);
        if (!ok || v < 0)
            return std::nullopt;
        values.append(v);
    }
    const int first = values.at(0);
    const int last = values.at(1);
    const int leadOut = values.at(2);
    if (first < 1 || last < first || last > 99 || values.size() != 3 + (last - first + 1))
        return std::nullopt;

    Toc toc;
    toc.source = u"TOC string"_s;
    for (int i = 0; i <= last - first; ++i) {
        Track t;
        t.number = first + i;
        t.firstLba = values.at(3 + i) - PregapSectors;
        const int nextAbsolute = (i < last - first) ? values.at(4 + i) : leadOut;
        t.lastLba = nextAbsolute - PregapSectors - 1;
        toc.tracks.append(t);
    }
    toc.leadOutLba = leadOut - PregapSectors;
    if (!toc.isValid())
        return std::nullopt;
    return toc;
}

}
