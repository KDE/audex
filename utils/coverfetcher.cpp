/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2013 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
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

CoverFetcher::CoverFetcher(QObject *parent) : QObject(parent) {
  Q_UNUSED(parent);
  _status = NOS;
  f_i = 0;
}

CoverFetcher::~CoverFetcher() {
  clear();
}

void CoverFetcher::startFetchThumbnails(const QString& searchstring, const int fetchNo) {

  if (_status != NOS) return;

  if (fetchNo == 0) return;

  fetch_no = fetchNo;

  QString ss = searchstring;
  ss.replace("&", "");

  QString url;
  url = "http://images.google.com/images?gbv=1&q=" + KUrl::toPercentEncoding(ss, "/");

  kDebug() << "searching covers (" << url << ")...";

  _status = SEARCHING; emit statusChanged(SEARCHING);

  job = KIO::storedGet(url);
  connect(job, SIGNAL(result(KJob*)), SLOT(fetched_html_data(KJob*)));

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
    kDebug() << "There was an error communicating with Google.";
    emit error(i18n("There was an error communicating with Google."), i18n("Try again later. Otherwise make a bug report."));
    _status = NOS; emit statusChanged(NOS);
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

  QRegExp rx("<a\\shref=\"(\\/imgres\\?imgurl=[^\"]+)\">[\\s\\n]*<img[^>]+src=\"([^>]+)\"></a>");
  QString html = xml;
  html.replace(QLatin1String("&amp;"), QLatin1String("&"));
  rx.setMinimal(TRUE);
  
  int pos = 0; int i = 0;
  while (((pos = rx.indexIn(xml, pos)) != -1) && (i < fetch_no)) {

    KUrl url("http://www.google.com"+rx.cap(1));
    cover_urls << url.queryItemValue("imgurl");
    QString w = url.queryItemValue("w");
    QString h = url.queryItemValue("h");
    QString sz = url.queryItemValue("sz");
    cover_names << i18n("%1x%2, %3 KiB", w, h, sz);
    cover_tbnids << url.queryItemValue("tbnid");
    
    if (!rx.cap(2).isEmpty()) {
      cover_urls_thumbnails << rx.cap(2);
    } else {
      cover_urls_thumbnails << cover_urls.last();
    }
    
    pos += rx.matchedLength();
    
    ++i;
    
  }

}

bool CoverFetcher::fetch_cover_thumbnail() {

  if (cover_urls_thumbnails.count() == 0) {
    emit nothingFetched();
    return FALSE;
  }

  _status = FETCHING_THUMBNAIL; emit statusChanged(FETCHING_THUMBNAIL);

  job = KIO::storedGet(KUrl(cover_urls_thumbnails.takeFirst()));
  connect(job, SIGNAL(result(KJob*)), SLOT(fetched_html_data(KJob*)));

  return TRUE;

}
