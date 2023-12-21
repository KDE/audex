/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef COVERFETCHER_HEADER
#define COVERFETCHER_HEADER

#include <QByteArray>
#include <QObject>
#include <QRegExp>

#include <KIO/Job>
#include <KIO/SimpleJob>
#include <KIO/TransferJob>
#include <KLocalizedString>

class CoverFetcher : public QObject
{
    Q_OBJECT
public:
    explicit CoverFetcher(QObject *parent = nullptr);
    ~CoverFetcher() override;

    void startFetchThumbnails(const QString &searchstring, const int fetchNo = 8);
    void stopFetchThumbnails();
    void startFetchCover(const int no);

    const QByteArray thumbnail(int index);
    const QString caption(int index);
    const QString tbnid(int index);
    inline int count()
    {
        return cover_names.count();
    }

    enum Status { NOS, SEARCHING, FETCHING_THUMBNAIL, FETCHING_COVER };

    inline Status status() const
    {
        return _status;
    }

Q_SIGNALS:
    void fetchedThumbnail(const QByteArray &thumbnail, const QString &caption, int no);
    void allCoverThumbnailsFetched();
    void fetchedCover(const QByteArray &cover);
    void nothingFetched();

    void statusChanged(CoverFetcher::Status status);

    void error(const QString &description, const QString &solution = QString());
    void warning(const QString &description);
    void info(const QString &description);

private Q_SLOTS:
    void fetched_html_data(KJob *job);
    void fetched_external_ip(KJob *job);

private:
    int fetch_no;
    QStringList cover_urls_thumbnails;
    QStringList cover_urls;
    QStringList cover_names;
    QStringList cover_tbnids;
    QList<QByteArray> cover_thumbnails;
    void clear()
    {
        cover_thumbnails.clear();
    }

    KIO::TransferJob *job;

    Status _status;

    int f_i;
    QString external_ip;
    QString search_string;

    void parse_html_response(const QString &html);
    bool fetch_cover_thumbnail();
    bool fetch_cover(const int no);
};

#endif
