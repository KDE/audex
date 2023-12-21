/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Some of this code is inspired by amarok 1.4.7
 * SPDX-FileCopyrightText: 2004 Mark Kretschmann <markey@web.de>
 * SPDX-FileCopyrightText: 2004 Stefan Bogner <bochi@online.ms>
 * SPDX-FileCopyrightText: 2004 Max Howell
 */

#include "coverfetcher.h"
#include <QDebug>
#include <QScriptEngine>
#include <QScriptValueIterator>
#include <algorithm>

CoverFetcher::CoverFetcher(QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(parent);
    _status = NOS;
    f_i = 0;
}

CoverFetcher::~CoverFetcher()
{
    clear();
}

void CoverFetcher::fetched_external_ip(KJob *job)
{
    qDebug() << "got IP...";
    if (!job) {
        qDebug() << "no job error ...";
        Q_EMIT nothingFetched();
        return;
    } else if (job && job->error()) {
        qDebug() << "reply error ...";
        Q_EMIT nothingFetched();
        return;
    }
    // http://www.telize.com/ip returns plaintext ip address
    auto *const storedJob = static_cast<KIO::StoredTransferJob *>(job);
    external_ip = ((QString)storedJob->data()).trimmed();

    qDebug() << "IP " << external_ip;

    // Max images per request on Google API is 8, thus the std::min
    QString url;
    url = QString("https://ajax.googleapis.com/ajax/services/search/images?v=1.0&q=%1&rsz=%2&userip=%3")
              .arg(QUrl::toPercentEncoding(search_string, "/").constData())
              .arg(std::min(fetch_no, 8))
              .arg(QUrl::toPercentEncoding(external_ip).constData());

    qDebug() << "searching covers (" << url << ")...";

    _status = SEARCHING;
    Q_EMIT statusChanged(SEARCHING);

    job = KIO::storedGet(url);
    connect(job, SIGNAL(result(KJob *)), SLOT(fetched_html_data(KJob *)));
}

void CoverFetcher::startFetchThumbnails(const QString &searchstring, const int fetchNo)
{
    qDebug() << "Fetch Thumbs ...";
    if (_status != NOS || fetchNo == 0) {
        Q_EMIT nothingFetched();
        return;
    }

    fetch_no = fetchNo;

    search_string = searchstring;
    search_string.remove('&');

    // Google requires the user IP
    QString url("http://www.telize.com/ip");

    job = KIO::storedGet(url);
    connect(job, SIGNAL(result(KJob *)), SLOT(fetched_external_ip(KJob *)));
}

void CoverFetcher::stopFetchThumbnails()
{
    if ((_status != FETCHING_THUMBNAIL) && (_status != SEARCHING))
        return;

    if (job)
        job->kill();

    _status = NOS;
    Q_EMIT statusChanged(NOS);
}

void CoverFetcher::startFetchCover(const int no)
{
    if (_status != NOS)
        return;

    if ((cover_urls.count() == 0) || (no >= cover_urls.count()) || (no < 0)) {
        Q_EMIT nothingFetched();
        return;
    }

    qDebug() << "fetching cover...";
    _status = FETCHING_COVER;
    Q_EMIT statusChanged(FETCHING_COVER);

    job = KIO::storedGet(QUrl(cover_urls[no]));
    connect(job, SIGNAL(result(KJob *)), SLOT(fetched_html_data(KJob *)));
}

const QByteArray CoverFetcher::thumbnail(int index)
{
    if ((index < 0) || (index >= cover_thumbnails.count()))
        return QByteArray();
    return cover_thumbnails[index];
}

const QString CoverFetcher::caption(int index)
{
    if ((index < 0) || (index >= cover_names.count()))
        return QString();
    return cover_names[index];
}

void CoverFetcher::fetched_html_data(KJob *job)
{
    QByteArray buffer;

    if (job && job->error()) {
        qDebug() << "There was an error communicating with Google. " << job->errorString();
        Q_EMIT error(i18n("There was an error communicating with Google."), i18n("Try again later. Otherwise make a bug report."));
        _status = NOS;
        Q_EMIT statusChanged(NOS);
        Q_EMIT nothingFetched();
        return;
    }
    if (job) {
        auto *const storedJob = static_cast<KIO::StoredTransferJob *>(job);
        buffer = storedJob->data();
    }

    if (buffer.count() == 0) {
        qDebug() << "Google server: empty response";
        Q_EMIT error(i18n("Google server: Empty response."), i18n("Try again later. Make a bug report."));
        _status = NOS;
        Q_EMIT statusChanged(NOS);
        return;
    }

    switch (_status) {
    case SEARCHING: {
        qDebug() << "searching finished.";
        // qDebug() << QString::fromUtf8(buffer.data());
        parse_html_response(QString::fromUtf8(buffer.data()));
        _status = NOS;
        Q_EMIT statusChanged(NOS);
        fetch_cover_thumbnail();
    } break;

    case FETCHING_THUMBNAIL: {
        qDebug() << "cover thumbnail fetched.";
        cover_thumbnails.append(buffer);
        Q_EMIT fetchedThumbnail(buffer, cover_names[f_i], f_i + 1);
        ++f_i;
        if (((fetch_no > -1) && (f_i == fetch_no)) || (cover_urls_thumbnails.count() == 0)) {
            _status = NOS;
            Q_EMIT statusChanged(NOS);
            f_i = 0;
            Q_EMIT allCoverThumbnailsFetched();
        } else {
            fetch_cover_thumbnail();
        }
    } break;

    case FETCHING_COVER: {
        qDebug() << "cover fetched.";
        _status = NOS;
        Q_EMIT statusChanged(NOS);
        Q_EMIT fetchedCover(buffer);
    } break;

    case NOS:
        break;
    }
}

void CoverFetcher::parse_html_response(const QString &xml)
{
    cover_urls_thumbnails.clear();
    cover_urls.clear();
    cover_names.clear();
    cover_tbnids.clear();
    cover_thumbnails.clear();

    QScriptValue responseData;
    QScriptEngine engine;
    responseData = engine.evaluate('(' + xml + ')');

    QScriptValue resultsData = responseData.property("responseData").property("results");

    if (resultsData.isArray()) {
        QScriptValueIterator it(resultsData);

        while (it.hasNext()) {
            it.next();
            if (it.flags() & QScriptValue::SkipInEnumeration)
                continue;

            QScriptValue entry = it.value();

            QString link = QUrl::fromPercentEncoding(entry.property("url").toString().toLatin1());
            QString thumbUrl = QUrl::fromPercentEncoding(entry.property("tbUrl").toString().toLatin1());
            QString w = entry.property("width").toString();
            QString h = entry.property("height").toString();

            cover_urls << link;
            cover_names << i18n("%1x%2", w, h);
            cover_urls_thumbnails << thumbUrl;

            qDebug() << "URL " << link << "- " << thumbUrl << " -" << cover_names;
        }
    }
}

bool CoverFetcher::fetch_cover_thumbnail()
{
    if (cover_urls_thumbnails.count() == 0) {
        qDebug() << "nothing fetched.";
        Q_EMIT nothingFetched();
        return false;
    }

    _status = FETCHING_THUMBNAIL;
    Q_EMIT statusChanged(FETCHING_THUMBNAIL);

    job = KIO::storedGet(QUrl(cover_urls_thumbnails.takeFirst()));
    connect(job, SIGNAL(result(KJob *)), SLOT(fetched_html_data(KJob *)));

    return true;
}
