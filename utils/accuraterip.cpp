/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "accuraterip.h"

namespace Audex
{

namespace AccurateRip
{

quint32 Id1(const Toc::Toc &toc)
{
    TrackOffsetList list = toc.trackOffsetList(false, true);

    quint32 id = 0;
    for (int i = 0; i < list.count(); ++i)
        id += list.at(i);
    return id;
}

quint32 Id2(const Toc::Toc &toc)
{
    TrackOffsetList list = toc.trackOffsetList(false, true);

    quint32 id = 0;
    for (int i = 0; i < list.count(); ++i)
        id += (list.at(i) > 0 ? list.at(i) : 1) * (quint32)(i + 1);
    return id;
}

QByteArray Id(const Toc::Toc &toc)
{
    QByteArray track_count = QByteArray::number(toc.audioTrackCount(), 10);
    while (track_count.length() < 3)
        track_count.prepend('0');
    QByteArray id1 = QByteArray::number(Id1(toc), 16);
    while (id1.length() < 8)
        id1.prepend('0');
    QByteArray id2 = QByteArray::number(Id2(toc), 16);
    while (id2.length() < 8)
        id2.prepend('0');
    QByteArray cddbid = QByteArray::number(CDDBId(toc), 16);
    while (cddbid.length() < 8)
        cddbid.prepend('0');
    return track_count + "-" + id1 + "-" + id2 + "-" + cddbid;
}

ChecksumCalculator::ChecksumCalculator()
{
    reset();
}

quint32 ChecksumCalculator::result() const
{
    return checksum_lo;
}

quint32 ChecksumCalculator::result450() const
{
    return checksum_450;
}

quint32 ChecksumCalculator::resultV2() const
{
    return checksum_lo + checksum_hi;
}

void ChecksumCalculator::reset()
{
    checksum_lo = 0;
    checksum_hi = 0;
    checksum_450 = 0;
    index = 1;
}

void ChecksumCalculator::process(const QByteArray &data, const int skipSamples, const int chopSamples)
{

    if (data.size() % 4) {
        qDebug() << "Warning: Unexpected buffer size:" << data.size();
        return;
    }

    int length = data.length() / 4;
    const quint32 *buffer = reinterpret_cast<const quint32 *>(data.constData());

    if (length < skipSamples || length < chopSamples)
        return;

    if (skipSamples > 0) {
        buffer += skipSamples;
        length -= skipSamples;
        index += skipSamples;
    }

    if (chopSamples > 0)
        length -= chopSamples;

    while (length > 0) {

        quint64 checksum64 = (quint64)*buffer * (quint64)index;

        checksum_hi += (quint32)(checksum64 >> 32);
        checksum_lo += (quint32)checksum64;

        if (index > 450 * CD_SECTORSIZE_SAMPLES && index <= 451 * CD_SECTORSIZE_SAMPLES)
            checksum_450 += *buffer * (index - (450 * CD_SECTORSIZE_SAMPLES));

        ++buffer;
        --length;

        ++index;
    }

}

Track::Track(QDataStream &bin)
{
    read(bin);
}

void Track::read(QDataStream &bin)
{
    clear();

    quint8 confidence;
    quint32 checksum;
    quint32 checksum450;

    bin >> confidence >> checksum >> checksum450;

    set(Confidence, confidence);
    set(Checksum1, checksum);
    set(Checksum450, checksum450);
}


Record::Record(QDataStream &bin)
{
    read(bin);
}

Record::Record(const Record &other) : Dataset(other)
{
    tracks = other.tracks;
}

Record &Record::operator=(const Record &other)
{
    Dataset::operator=(other);
    tracks = other.tracks;
    return *this;
}

const Track &Record::track(const int tracknumber) const
{
    return tracks.at(tracknumber - 1);
}

int Record::appendTrack(const Track &track)
{
    tracks.append(track);
    return tracks.length();
}

int Record::trackCount() const
{
    return tracks.count();
}

void Record::clear()
{
    Dataset::clear();
    tracks.clear();
}

void Record::read(QDataStream &bin)
{
    clear();

    quint8 track_count;
    quint32 id1;
    quint32 id2;
    quint32 cddbid;

    bin >> track_count >> id1 >> id2 >> cddbid;

    set(ID1, id1);
    set(ID2, id2);
    set(CddbID, cddbid);

    for (int i = 0; i < track_count; ++i)
        tracks.append(Track(bin));

}

Records::Records()
{

}

Records::Records(const Records &other)
{
    records = other.records;
    last_error = other.last_error;
}

Records &Records::operator=(const Records &other)
{
    records = other.records;
    last_error = other.last_error;
    return *this;
}

const Record &Records::record(const int recordnumber) const
{
    return records.at(recordnumber);
}

int Records::appendRecord(const Record &record)
{
    records.append(record);
    return records.length();
}

int Records::recordCount() const
{
    return records.count();
}

Message Records::lastError() const
{
    return last_error;
}

void Records::resetError()
{
    last_error.clear();
}

void Records::clear()
{
    records.clear();
}

bool Records::isEmpty() const
{
    return records.count() == 0;
}

int Records::count() const
{
    return records.count();
}

bool Records::read(const QByteArray &data, const int expectedTrackCount)
{
    resetError();

    int record_size = 1 + 12 + expectedTrackCount * (1 + 8);
    if (data.size() % record_size) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Wrong AccurateRip reply size:" << data.size();
        last_error = Message(i18n("Failed to parse reply from AccurateRip database."), Message::ERROR);
        return false;
    }

    QDataStream bin(data);
    return read(bin, data.size() / record_size);

}

bool Records::read(QDataStream &bin, const int record_count)
{

    records.clear();

    bin.setByteOrder(QDataStream::LittleEndian);

    for (int i = 0; i < record_count; ++i)
        records.append(Record(bin));

    return true;

}

QDebug operator<<(QDebug debug, const Records &records)
{
    QDebugStateSaver saver(debug);

    for (int i = 0; i < records.recordCount(); ++i) {
        debug.nospace().noquote() << "--- Record number: " << i << Qt::endl;
        debug.nospace().noquote() << "Number of tracks: " << records.record(i).trackCount() << Qt::endl;
        debug.nospace().noquote() << "AccurateRipId 1: " << QStringLiteral(u"%1").arg(records.record(i).get(ID1).toUInt(), 8, 16, QChar(u'0')) << Qt::endl;
        debug.nospace().noquote() << "AccurateRipId 2: " << QStringLiteral(u"%1").arg(records.record(i).get(ID2).toUInt(), 8, 16, QChar(u'0')) << Qt::endl;
        debug.nospace().noquote() << "CDDBId: " << QStringLiteral(u"%1").arg(records.record(i).get(CddbID).toUInt(), 8, 16, QChar(u'0')) << Qt::endl;

        for (int j = 1; j <= records.record(i).trackCount(); ++j) {
            debug.nospace().noquote() << "Track: " << j << ", Confidence: " << records.record(i).track(j).get(Confidence).toUInt() << ", Checksum: " << QStringLiteral(u"%1").arg(records.record(i).track(j).get(Checksum1).toUInt(), 8, 16, QChar(u'0'))
                 << ", Checksum 450: " << QStringLiteral(u"%1").arg(records.record(i).track(j).get(Checksum450).toUInt(), 8, 16, QChar(u'0')) << Qt::endl;
        }

    }

    return debug;
}

Database::Database(QObject *parent, const bool cacheEnabled) : QObject(parent)
{
    qRegisterMetaType<Records>("Records");

    p_cache_enabled = cacheEnabled;
    p_nam = new QNetworkAccessManager(parent);
    connect(p_nam, &QNetworkAccessManager::finished, this, &Database::p_reply_finished);

    p_cache_path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).at(0) + QStringLiteral(u"/accuraterip");

    if (cacheEnabled) {
        QDir dir;
        if (!dir.mkpath(p_cache_path)) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Unable to create AccurateRip cache dir, disabling cache:" << p_cache_path;
            p_cache_enabled = false;
        }
    }

    is_retrieving = false;

}

Database::~Database()
{
    if (p_nam) {
        delete p_nam;
        p_nam = nullptr;
    }
}

Message Database::lastError() const
{
    return last_error;
}

void Database::resetError()
{
    last_error.clear();
}

void Database::lookup(const QByteArray &id, const int expectedTrackCount)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << id;

    if (is_retrieving) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Retrieving records from AccurateRip database already running.";
        return;
    }

    is_retrieving = true;

    if (p_cache_enabled) {

        QString filepath = QStringLiteral(u"%1/%2.bin").arg(p_cache_path).arg(QString::fromLatin1(id));

        if (QFileInfo::exists(filepath)) {
            QFile file(filepath);
            if (file.open(QIODevice::ReadOnly)) {

                qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Found AccurateRip entry in local cache:" << filepath;

                Records records = parse_records(file.readAll(), expectedTrackCount);

                file.close();

                if (records.isEmpty()) {
                    Q_EMIT lookupFinished(false, id, Records());
                    Q_EMIT log(id, last_error);
                } else {
                    Q_EMIT lookupFinished(true, id, records);
                }

                is_retrieving = false;
                return;
            }
        }

    }

    QUrl url(QStringLiteral(u"https://www.accuraterip.com/accuraterip/%1/%2/%3/dBAR-%4.bin").arg(QLatin1Char(id.at(11))).arg(QLatin1Char(id.at(10))).arg(QLatin1Char(id.at(9))).arg(QString::fromLatin1(id)));
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Starting AccurateRip request:" << url;

    Q_EMIT log(id, Message(i18n("Request AccurateRip database with ID %1", QString::fromLatin1(id))));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral(u"curl/8.6.0"));

    retrieving_id = id;
    retrieving_expected_track_count = expectedTrackCount;

    p_nam->get(request);

}

void Database::p_reply_finished(QNetworkReply *reply)
{

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    QByteArray id = retrieving_id;
    int expected_track_count = retrieving_expected_track_count;

    is_retrieving = false;
    retrieving_id.clear();
    retrieving_expected_track_count = 0;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "No entry in AccurateRip database found or connection error:" << reply->error();

        if (reply->error() == QNetworkReply::ContentNotFoundError) {
            last_error = Message(i18n("No entry in Accurate Rip database found."), Message::ERROR, 0);
        } else {
            last_error = Message(i18n("Error while connecting to Accurate Rip database."), Message::ERROR, 1, reply->errorString());
        }
        Q_EMIT log(id, last_error);
        Q_EMIT lookupFinished(false, id, Records());
        return;
    }

    if (p_cache_enabled) {

        QString filepath = QStringLiteral(u"%1/%2.bin").arg(p_cache_path).arg(QString::fromLatin1(id));

        if (!QFileInfo::exists(filepath)) {
            QFile file(filepath);
            if (file.open(QIODevice::ReadWrite)) {
                qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Writing AccurateRip entry to local cache:" << filepath;
                file.write(reply->readAll());
                file.close();
            }
        }

    }

    Records records = parse_records(reply->readAll(), expected_track_count);

    if (records.isEmpty()) {
        Q_EMIT lookupFinished(false, id, Records());
        Q_EMIT log(id, last_error);
    } else {
        Q_EMIT lookupFinished(true, id, records);
    }

}

Records Database::parse_records(const QByteArray &data, const int expected_track_count)
{

    Records result;

    if (!result.read(data, expected_track_count))
        return Records();

    return result;

}

bool verify(const Records &records, const quint32 checksum, const int tracknumber, int *confidence)
{

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    for (int i = 0; i < records.count(); ++i) {

        quint32 c = records.record(i).track(tracknumber).get(Checksum1).toUInt();

        if (c == 0 || checksum == 0)
            continue;

        if (checksum == c) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Found AccurateRip match for track" << tracknumber << "at record number" << i << "with checksum" << QStringLiteral(u"%1").arg(c, 8, 16, QChar(u'0'));
            if (confidence)
                *confidence = records.record(i).track(tracknumber).get(Confidence).toUInt();
            return true;
        }

    }

    return false;

}

}

}
