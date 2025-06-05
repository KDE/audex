/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdtextreader.h"

#include "scsi.h"
#include "utils/crc.h"

namespace Audex
{

namespace Device
{

void CDTextReader::read(const QString &discUDI)
{
    disc_udi = discUDI;
    start();
}

void CDTextReader::run()
{
    if (isInterruptionRequested())
        return;

    Solid::Device device(disc_udi);
    QByteArray block_device = device.as<Solid::Block>()->device().toLatin1();

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << block_device;

    int deviceHandle = ::open(block_device.constData(), O_RDONLY | O_NONBLOCK);
    if (deviceHandle == -1) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to initialize device:" << block_device;
        last_error = i18n("Failed to initialize drive %1.", QString::fromLatin1(block_device));
        Q_EMIT finished(disc_udi, false);
        return;
    }

    if (isInterruptionRequested())
        return;

    SCSI::ErrorCode ec;
    QByteArray buffer = SCSI::readTocPmaAtip(deviceHandle, ec, 5, true, 0);

    if (ec) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to read CD-Text:" << block_device;
        last_error = i18n("Failed to read CD-Text from disc in drive %1: %2 (%3).", QString::fromLatin1(block_device), ec.senseKeyString(), ec.errorCode());
        ::close(deviceHandle);
        Q_EMIT finished(disc_udi, false);
        return;
    }

    if (isInterruptionRequested())
        return;

    // we need more than the header and a multiple of 18 bytes to have valid CD-TEXT
    if (buffer.size() <= 4 || buffer.size() % 18 != 4) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Invalid CD-Text length:" << buffer.size() << block_device;
        last_error = i18n("Failed to read CD-Text from disc in drive %1 (invalid length).", QString::fromLatin1(block_device));
        ::close(deviceHandle);
        Q_EMIT finished(disc_udi, false);
        return;
    }

    debug_raw_text_pack_data(buffer);

    int r = buffer.size() % 18;
    cdtext_pack *pack = (cdtext_pack *)&buffer.data()[r];

    QHash<int, QHash<int, QString>> tracks;

    QString payload;
    for (int i = 0; i < (buffer.size() - r) / 18; ++i) {
        if (pack[i].dbcc) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__
                     << "CD-Text decoding of disc in drive failed (Double byte code not supported yet):" << block_device;
            last_error = i18n("Failed to decode CD-Text from disc in drive %1 (double byte code not supported yet).", QString::fromLatin1(block_device));
            ::close(deviceHandle);
            Q_EMIT finished(disc_udi, false);
            return;
        }

        // For some reason all crc bits are inverted.
        pack[i].crc[0] ^= 0xff;
        pack[i].crc[1] ^= 0xff;

        quint16 crc = CRC::CRC16_X25_Calculator::calc(QByteArray(reinterpret_cast<const char *>(&pack[i]), 18));

        pack[i].crc[0] ^= 0xff;
        pack[i].crc[1] ^= 0xff;

        if (crc != 0x0000)
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "CD-Text checksum invalid for disc in drive:" << block_device << "continue anyway...";

        // pack.data has a length of 12
        //
        // id2 tells us the tracknumber of the data (0 for global)
        // data may contain multiple \0. In that case after every \0 the track number increases +1.

        const int id = pack[i].id1;
        int tracknumber = pack[i].id2;

        if (tracknumber == 0 && id == 0x87) { // genre
            quint16 genre_code = qFromBigEndian(*reinterpret_cast<const quint16 *>(pack[i].data));
            QString genre;
            switch (genre_code) {
            case 0x0000:
            case 0x0001:
                break;
            case 0x0002:
                genre = i18n("Adult Contemporary");
                break;
            case 0x0003:
                genre = i18n("Alternative Rock");
                break;
            case 0x0004:
                genre = i18n("Childrens' Music");
                break;
            case 0x0005:
                genre = i18n("Classical");
                break;
            case 0x0006:
                genre = i18n("Contemporary Christian");
                break;
            case 0x0007:
                genre = i18n("Country");
                break;
            case 0x0008:
                genre = i18n("Dance");
                break;
            case 0x0009:
                genre = i18n("Easy Listening");
                break;
            case 0x000a:
                genre = i18n("Erotic");
                break;
            case 0x000b:
                genre = i18n("Folk");
                break;
            case 0x000c:
                genre = i18n("Gospel");
                break;
            case 0x000d:
                genre = i18n("Hip Hop");
                break;
            case 0x000e:
                genre = i18n("Jazz");
                break;
            case 0x000f:
                genre = i18n("Latin");
                break;
            case 0x0010:
                genre = i18n("Musical");
                break;
            case 0x0011:
                genre = i18n("New Age");
                break;
            case 0x0012:
                genre = i18n("Opera");
                break;
            case 0x0013:
                genre = i18n("Operetta");
                break;
            case 0x0014:
                genre = i18n("Pop Music");
                break;
            case 0x0015:
                genre = i18n("Rap");
                break;
            case 0x0016:
                genre = i18n("Reggae");
                break;
            case 0x0017:
                genre = i18n("Rock Music");
                break;
            case 0x0018:
                genre = i18n("Rhythm & Blues");
                break;
            case 0x0019:
                genre = i18n("Sound Effects");
                break;
            case 0x001a:
                genre = i18n("Spoken Word");
                break;
            case 0x001b:
                genre = i18n("World Music");
                break;
            }

            QString genre_supp;
            char *pos = (char *)pack[i].data + 2;
            while (*pos != 0) {
                genre_supp.append(QLatin1Char(*pos));
                ++pos;
            }

            if (!genre.isEmpty() && genre_supp.isEmpty())
                tracks[tracknumber][id] = genre;
            else if (genre.isEmpty() && !genre_supp.isEmpty())
                tracks[tracknumber][id] = genre_supp;
            else if (!genre.isEmpty() && !genre_supp.isEmpty())
                tracks[tracknumber][id] = genre + QStringLiteral(u" (") + genre_supp + QStringLiteral(u")");

            continue;
        }

        if (id == 0x80 || id == 0x81 || id == 0x82 || id == 0x83 || id == 0x84 || id == 0x85 || id == 0x86 || id == 0x8e) {
            char *pos = (char *)pack[i].data;
            for (int j = 0; j < 12; ++j) {
                if (*pos != '\0') {
                    payload.append(QLatin1Char(*pos));
                } else {
                    if (payload == QStringLiteral(u"\t"))
                        payload = tracks[tracknumber - 1][id];
                    tracks[tracknumber][id].append(payload);
                    payload.clear();
                    ++tracknumber;
                }
                ++pos;
            }
        }
    }

    // Cache -> Metadata

    if (isInterruptionRequested())
        return;

    Metadata::Metadata cdtext;

    QList<int> tracknumbers = tracks.keys();
    std::sort(tracknumbers.begin(), tracknumbers.end());

    if (!tracks.value(0).value(0x80).simplified().isEmpty())
        cdtext.set(Metadata::Album, tracks.value(0).value(0x80)); // TITLE

    if (!tracks.value(0).value(0x81).simplified().isEmpty())
        cdtext.set(Metadata::Artist, tracks.value(0).value(0x81)); // PERFORMER

    if (!tracks.value(0).value(0x82).simplified().isEmpty())
        cdtext.setCustom(QStringLiteral(u"Songwriter"), tracks.value(0).value(0x82)); // SONGWRITER

    if (!tracks.value(0).value(0x83).simplified().isEmpty())
        cdtext.set(Metadata::Composer, tracks.value(0).value(0x83)); // COMPOSER

    if (!tracks.value(0).value(0x84).simplified().isEmpty())
        cdtext.setCustom(QStringLiteral(u"Arranger"), tracks.value(0).value(0x84)); // ARRANGER

    if (!tracks.value(0).value(0x85).simplified().isEmpty())
        cdtext.set(Metadata::Comment, tracks.value(0).value(0x85)); // MESSAGE

    if (!tracks.value(0).value(0x86).simplified().isEmpty())
        cdtext.setCustom(QStringLiteral(u"CD-Text DISCID"), tracks.value(0).value(0x86)); // DISC IDENTIFICATION

    if (!tracks.value(0).value(0x8e).simplified().isEmpty())
        cdtext.set(Metadata::MCN, tracks.value(0).value(0x8e)); // UPC_EAN -> MCN

    for (int i = 1; i < tracknumbers.count(); ++i) {
        int tracknumber = tracknumbers.at(i);
        Metadata::Track track;
        if (!tracks.value(tracknumber).value(0x80).simplified().isEmpty())
            track.set(Metadata::Title, tracks.value(tracknumber).value(0x80)); // TITLE
        if (!tracks.value(tracknumber).value(0x81).simplified().isEmpty())
            track.set(Metadata::Artist, tracks.value(tracknumber).value(0x81)); // PERFORMER
        if (!tracks.value(tracknumber).value(0x82).simplified().isEmpty())
            track.setCustom(QStringLiteral(u"Songwriter"), tracks.value(tracknumber).value(0x82)); // SONGWRITER
        if (!tracks.value(tracknumber).value(0x83).simplified().isEmpty())
            track.set(Metadata::Composer, tracks.value(tracknumber).value(0x83)); // COMPOSER
        if (!tracks.value(tracknumber).value(0x84).simplified().isEmpty())
            track.setCustom(QStringLiteral(u"Arranger"), tracks.value(tracknumber).value(0x84)); // ARRANGER
        if (!tracks.value(tracknumber).value(0x85).simplified().isEmpty())
            track.set(Metadata::Comment, tracks.value(tracknumber).value(0x85)); // MESSAGE
        if (!tracks.value(tracknumber).value(0x8e).simplified().isEmpty())
            track.set(Metadata::ISRC, tracks.value(tracknumber).value(0x8e)); // UPC_EAN -> ISRC
        if (!track.isEmpty()) {
            track.set(Metadata::TrackNumber, tracknumber); // TRACK NUMBER
            cdtext.appendTrack(track);
        }
    }

    cdtext.confirm();

    ::close(deviceHandle);

    Q_EMIT finished(disc_udi, true, cdtext);
}

void CDTextReader::debug_raw_text_pack_data(const QByteArray &data)
{
    qDebug().nospace().noquote() << " id1    | id2    | id3    | charps | blockn | dbcc | data           | crc |";

    cdtext_pack *pack = (cdtext_pack *)data.data();

    for (int i = 0; i < data.size() / 18; ++i) {
        QString s;
        s += QStringLiteral(u" %1 |").arg(pack[i].id1, 6, 16);
        s += QStringLiteral(u" %1 |").arg(pack[i].id2, 6);
        s += QStringLiteral(u" %1 |").arg(pack[i].id3, 6);
        s += QStringLiteral(u" %1 |").arg(pack[i].charpos, 6);
        s += QStringLiteral(u" %1 |").arg(pack[i].blocknum, 6);
        s += QStringLiteral(u" %1 |").arg(pack[i].dbcc, 4);
        qDebug().nospace().noquote() << s;
    }
}

}

}
