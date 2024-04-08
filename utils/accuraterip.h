/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ACCURATERIP_H
#define ACCURATERIP_H

#include <QObject>
#include <QDebug>
#include <QDataStream>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

#include <datatypes/toc.h>
#include <datatypes/message.h>
#include <utils/cddbid.h>

namespace Audex
{

namespace AccurateRip
{

quint32 Id1(const Toc::Toc &toc);
quint32 Id2(const Toc::Toc &toc);
QByteArray Id(const Toc::Toc &toc);

class ChecksumCalculator
{

public:
    ChecksumCalculator();

    quint32 result() const;
    quint32 result450() const;
    quint32 resultV2() const;

    void reset();

    void process(const QByteArray &data, const int skipSamples = 0, const int chopSamples = 0);

private:
    quint32 checksum_hi;
    quint32 checksum_lo;

    quint32 checksum_450;

    int index;
};

enum Type { RecordNumber = 0, ID1, ID2, CddbID, Tracknumber, Confidence, Checksum1, Checksum450, Invalid };

class Track : public Dataset
{

public:
    Track(QDataStream &bin);

    void read(QDataStream &bin);
};

typedef QList<Track> TrackList;

class Record : public Dataset
{

public:
    Record(QDataStream &bin);
    Record(const Record &other);
    Record &operator=(const Record &other);

    const Track &track(const int tracknumber) const;
    int appendTrack(const Track &track); // return tracknumber of appended track
    int trackCount() const;

    void clear();

    void read(QDataStream &bin);

private:
    TrackList tracks;
};

typedef QList<Record> RecordList;

class Records
{

public:
    Records();
    Records(const Records &other);
    Records &operator=(const Records &other);

    const Record &record(const int recordnumber) const;
    int appendRecord(const Record &record); // return recordnumber of appended record
    int recordCount() const;

    Message lastError() const;
    void resetError();

    void clear();

    bool isEmpty() const;

    int count() const;

    bool read(const QByteArray &data, const int expectedTrackCount);

private:
    RecordList records;

    bool read(QDataStream &bin, const int record_count);

    Message last_error;
};

QDebug operator<<(QDebug debug, const Records &records);

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr, const bool cacheEnabled = true);
    ~Database();

    Message lastError() const;
    void resetError();

    void lookup(const QByteArray& id, const int expectedTrackCount);

    bool isRetrieving() const;

Q_SIGNALS:
    void lookupFinished(const bool successful, const QByteArray& id, const Records& records);

    void log(const QByteArray &id, const Message &msg);

private Q_SLOTS:
    void p_reply_finished(QNetworkReply *reply);

private:
    Records parse_records(const QByteArray& data, const int expected_track_count);

    QNetworkAccessManager *p_nam;

    bool is_retrieving;
    QByteArray retrieving_id;
    int retrieving_expected_track_count;

    bool p_cache_enabled;
    QString p_cache_path;

    Message last_error;
};

bool verify(const Records& records, const quint32 checksum, const int tracknumber, int *confidence = nullptr);

}

}

#endif
