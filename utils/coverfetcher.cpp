/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <http://userbase.kde.org/Audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Some of this code is inspired by amarok 1.4.7
 * (C) 2004 Mark Kretschmann <markey@web.de>
 * (C) 2004 Stefan Bogner <bochi@online.ms>
 * (C) 2004 Max Howell
*/

#include "coverfetcher.h"
#include <algorithm>
#include <QScriptEngine>
#include <QScriptValueIterator>

CoverFetcher::CoverFetcher(QObject *parent) : QObject(parent) {
  Q_UNUSED(parent);
  _status = NOS;
  f_i = 0;
}

CoverFetcher::~CoverFetcher() {
  clear();
}

void CoverFetcher::fetched_external_ip(KJob* job) {

  kDebug() << "got IP...";
  if (!job) {
    kDebug() << "no job error ...";
    emit nothingFetched();
    return;
  } else if (job && job->error()) {
    kDebug() << "reply error ...";
    emit nothingFetched();
    return;
  }
  // http://www.telize.com/ip returns plaintext ip address
  KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>(job);
  external_ip = ((QString) storedJob->data()).trimmed();

  kDebug() << "IP " << external_ip;

  // Max images per request on Google API is 8, thus the std::min
  QString url;
  url= QString("https://ajax.googleapis.com/ajax/services/search/images?v=1.0&q=%1&rsz=%2&userip=%3")
          .arg(KUrl::toPercentEncoding(search_string, "/").data())
          .arg(std::min(fetch_no,8))
          .arg(KUrl::toPercentEncoding(external_ip).data());

  kDebug() << "searching covers (" << url << ")...";

  _status = SEARCHING;
  emit statusChanged(SEARCHING);

  job = KIO::storedGet(url);
  connect(job, SIGNAL(result(KJob*)), SLOT(fetched_html_data(KJob*)));

}

void CoverFetcher::startFetchThumbnails(const QString& searchstring, const int fetchNo) {

  kDebug() << "Fetch Thumbs ...";
  if (_status != NOS || fetchNo == 0)
  {
      emit nothingFetched();
      return;
  }

  fetch_no = fetchNo;

  search_string = searchstring;
  search_string.replace("&", "");

  // Google requires the user IP
  QString url("http://www.telize.com/ip");

  job = KIO::storedGet(url);
  connect(job, SIGNAL(result(KJob*)), SLOT(fetched_external_ip(KJob*)));

}

void CoverFetcher::stopFetchThumbnails() {

  if ((_status != FETCHING_THUMBNAIL) && (_status != SEARCHING)) return;

  if (job) job->kill();

  _status = NOS; emit statusChanged(NOS);

}

void CoverFetcher::startFetchCover(const int no) {

  if (_status != NOS) return;

  if ((cover_urls.count()==0) || (no >= cover_urls.count()) || (no < 0)) {
    emit nothingFetched();
    return;
  }

  kDebug() << "fetching cover...";
  _status = FETCHING_COVER; emit statusChanged(FETCHING_COVER);

  job = KIO::storedGet(KUrl(cover_urls[no]));
  connect(job, SIGNAL(result(KJob*)), SLOT(fetched_html_data(KJob*)));

}

const QByteArray CoverFetcher::thumbnail(int index) {
  if ((index < 0) || (index >= cover_thumbnails.count())) return QByteArray();
  return cover_thumbnails[index];
}

const QString CoverFetcher::caption(int index) {
  if ((index < 0) || (index >= cover_names.count())) return QString();
  return cover_names[index];
}

void CoverFetcher::fetched_html_data(KJob* job) {

  QByteArray buffer;

  if (job && job->error()) {
    kDebug() << "There was an error communicating with Google. "<< job->errorString();
    emit error(i18n("There was an error communicating with Google."), i18n("Try again later. Otherwise make a bug report."));
    _status = NOS; emit statusChanged(NOS);
    emit nothingFetched();
    return;
  }
  if (job) {
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>(job);
    buffer = storedJob->data();
  }

  if (buffer.count() == 0) {
    kDebug() << "Google server: empty response";
    emit error(i18n("Google server: Empty response."),
               i18n("Try again later. Make a bug report."));
    _status = NOS;  emit statusChanged(NOS);
    return;
  }

  switch (_status) {

      case SEARCHING : {
        kDebug() << "searching finished.";
        //kDebug() << QString::fromUtf8(buffer.data());
        parse_html_response(QString::fromUtf8(buffer.data()));
        _status = NOS; emit statusChanged(NOS);
        fetch_cover_thumbnail();
      } break;

      case FETCHING_THUMBNAIL : {
        kDebug() << "cover thumbnail fetched.";
        cover_thumbnails.append(buffer);
        emit fetchedThumbnail(buffer, cover_names[f_i], f_i+1);
        ++f_i;
        if (((fetch_no > -1) && (f_i == fetch_no)) || (cover_urls_thumbnails.count() == 0)) {
          _status = NOS; emit statusChanged(NOS);
          f_i = 0;
          emit allCoverThumbnailsFetched();
        } else {
          fetch_cover_thumbnail();
        }
      } break;

      case FETCHING_COVER : {
        kDebug() << "cover fetched.";
	_status = NOS; emit statusChanged(NOS);
        emit fetchedCover(buffer);
      } break;

      case NOS : break;

  }


}

void CoverFetcher::parse_html_response(const QString& xml) {

  cover_urls_thumbnails.clear();
  cover_urls.clear();
  cover_names.clear();
  cover_tbnids.clear();
  cover_thumbnails.clear();

  QScriptValue responseData;
  QScriptEngine engine;
  responseData = engine.evaluate("("+xml+")");


  QScriptValue resultsData=responseData.property("responseData").property("results");

  if (resultsData.isArray()) {

    QScriptValueIterator it(resultsData);

    while (it.hasNext()) {

      it.next();
      if (it.flags() & QScriptValue::SkipInEnumeration) continue;

      QScriptValue entry = it.value();

      QString link =  QUrl::fromPercentEncoding(entry.property("url").toString().toAscii());
      QString thumbUrl = QUrl::fromPercentEncoding(entry.property("tbUrl").toString().toAscii());
      QString w = entry.property("width").toString();
      QString h = entry.property("height").toString();

      cover_urls << link;
      cover_names << i18n("%1x%2", w, h);
      cover_urls_thumbnails << thumbUrl;

      kDebug() << "URL " << link << "- " << thumbUrl<< " -"<<cover_names;

    }

  }
}

bool CoverFetcher::fetch_cover_thumbnail() {

  if (cover_urls_thumbnails.count() == 0) {
    kDebug() << "nothing fetched.";
    emit nothingFetched();
    return false;
  }

  _status = FETCHING_THUMBNAIL; emit statusChanged(FETCHING_THUMBNAIL);

  job = KIO::storedGet(KUrl(cover_urls_thumbnails.takeFirst()));
  connect(job, SIGNAL(result(KJob*)), SLOT(fetched_html_data(KJob*)));

  return true;

}
