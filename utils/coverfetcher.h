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
 
#ifndef COVERFETCHER_HEADER
#define COVERFETCHER_HEADER

#include <QObject>
#include <QByteArray>
#include <QRegExp>

#include <KDebug>
#include <KLocale>
#include <KUrl>
#include <KIO/Job>
#include <KIO/SimpleJob>
#include <KIO/TransferJob>

class CoverFetcher : public QObject {
  Q_OBJECT
public:
  CoverFetcher(QObject *parent = 0);
  ~CoverFetcher();

  void startFetchThumbnails(const QString& searchstring, const int fetchNo = 8);
  void stopFetchThumbnails();
  void startFetchCover(const int no);
  
  const QByteArray thumbnail(int index);
  const QString caption(int index);
  const QString tbnid(int index);
  inline int count() { return cover_names.count(); }

  enum Status {
    NOS,
    SEARCHING,
    FETCHING_THUMBNAIL,
    FETCHING_COVER
  };
  
  inline Status status() const { return _status; }

signals:
  void fetchedThumbnail(const QByteArray& thumbnail, const QString& caption, int no);
  void allCoverThumbnailsFetched();
  void fetchedCover(const QByteArray& cover);
  void nothingFetched();

  void statusChanged(Status status);

  void error(const QString& description,
	const QString& solution = QString());
  void warning(const QString& description);
  void info(const QString& description);

private slots:
  void fetched_html_data(KJob* job);

private:
  int fetch_no;
  QStringList cover_urls_thumbnails;
  QStringList cover_urls;
  QStringList cover_names;
  QStringList cover_tbnids;
  QList<QByteArray> cover_thumbnails;
  void clear() { cover_thumbnails.clear(); }

  KIO::TransferJob* job;

  Status _status;

  int f_i;

  void parse_html_response(const QString& html);
  bool fetch_cover_thumbnail();
  bool fetch_cover(const int no);

};

#endif
