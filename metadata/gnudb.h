/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// gnudb (the freedb successor, CDDB protocol level 6 over HTTP) - request
// building and response parsing. Pure functions, no network access, so they
// can be tested with fixtures.
//
// Protocol and format: https://gnudb.org/gnudbprotocol.php,
// https://gnudb.org/gnudbformat.php

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "candidate.h"
#include "cdinfo.h"

namespace Audex::Gnudb
{

inline constexpr auto DefaultServer = "https://gnudb.gnudb.org";
inline constexpr auto CgiPath = "/~cddb/cddb.cgi";
inline constexpr int ProtocolLevel = 6; // level 6 answers in UTF-8

// --- requests -------------------------------------------------------------

// gnudb requires a contact address and a descriptive client name in every
// request ("hello=name host client version"), else it blocks the client.
bool isUsableEmail(const QString &email);
// Empty if the address is unusable; the local part and the host of the
// address become the first two fields.
QString helloString(const QString &email, const QString &client, const QString &version);

// "cddb query <discid> <ntracks> <offset...> <seconds>". Empty if the disc has
// no TOC. All TOC tracks are listed, including data tracks: the CDDB disc id
// counts them as well.
QString queryCommand(const CDInfo &disc);

QUrl commandUrl(const QString &command, const QString &hello, const QUrl &server = QUrl(QString::fromLatin1(DefaultServer)));
QUrl queryUrl(const CDInfo &disc, const QString &hello, const QUrl &server = QUrl(QString::fromLatin1(DefaultServer)));
QUrl readUrl(const QString &category, const QString &discId, const QString &hello, const QUrl &server = QUrl(QString::fromLatin1(DefaultServer)));

// --- responses ------------------------------------------------------------

// A CDDB answer: a status line with a three digit code, optionally followed by
// data lines up to a line containing only ".". Level 6 answers are UTF-8;
// invalid UTF-8 is read as ISO-8859-1 (older entries).
struct Response {
    int code = 0;
    QString status; // the status line without the code
    QStringList lines; // data lines, without the terminating "."
};
Response parseResponse(const QByteArray &raw);

struct QueryMatch {
    QString category; // "rock", "misc", ... - needed for the read
    QString discId;
    QString title; // "Artist / Album", only for display
    bool exact = false;
};

struct QueryResult {
    QList<QueryMatch> matches;
    QString error; // protocol or server error; "no match" is not an error
};

// Codes: 200 one exact match (on the status line), 210 exact matches and 211
// inexact matches (as a list), 202 nothing found, 403/500 error.
QueryResult parseQueryResponse(const QByteArray &raw);

struct ReadResult {
    MetadataCandidate candidate;
    bool found = false;
    QString error;
};

// Turns the xmcd entry of a "cddb read" into a candidate. Entries whose track
// count fits neither the TOC nor the audio tracks of the disc are rejected
// (found = false, no error): the CDDB disc id is only 32 bits wide and
// collides now and then.
ReadResult parseReadResponse(const QByteArray &raw, const CDInfo &disc, const QString &category, const QString &discId, bool exact);

// The xmcd entry itself (the body of a read answer, a local file, ...).
ReadResult parseXmcd(const QStringList &lines, const CDInfo &disc, const QString &category, const QString &discId, bool exact);

// "a\\nb" -> "a<newline>b"; also "\\t" and "\\\\"
QString unescape(const QString &text);

// Artist and title of a DTITLE or TTITLE line: " / " separates them, and only
// that sequence does. Without it the whole text is the title.
struct TitleParts {
    QString artist;
    QString title;
};
TitleParts splitTitle(const QString &text);

}
