/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// This code is strongly inspired by K3B, 1998-2009 Sebastian Trueg <trueg@k3b.org>

#include "tocreader.h"

namespace Audex
{

namespace Device
{

// Read Raw TOC (format: 0010b)

// For POINT from 01h-63h we get all the tracks
// POINT a1h gices us the last track number in the session in PMIN
// POINT a2h gives the start of the session lead-out in PMIN,PSEC,PFRAME

const Toc::Toc readToc(const QString &driveUDI, QString &error)
{
    Solid::Device device(driveUDI);
    const QByteArray block_device = device.as<Solid::Block>()->device().toLatin1();

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << block_device;

    int deviceHandle = ::open(block_device.constData(), O_RDONLY | O_NONBLOCK);
    if (deviceHandle == -1) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to initialize device:" << block_device;
        error = i18n("Failed to initialize drive %1.", QString::fromLatin1(block_device));
        return Toc::Toc();
    }

    SCSI::ErrorCode ec;
    QByteArray buffer = SCSI::readTocPmaAtip(deviceHandle, ec);

    if (ec) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to read TOC:" << block_device;
        error = i18n("Failed to read TOC from disc in drive %1: %2", QString::fromLatin1(block_device), ec.senseKeyString());
        ::close(deviceHandle);
        return Toc::Toc();
    }

    track_descriptor_format_2h *tr = (track_descriptor_format_2h *)&buffer.data()[4];

    qDebug().nospace().noquote() << "Session |  ADR   | CONTROL|  TNO   | POINT  |  Min   |  Sec   | Frame  |  Zero  |  PMIN  |  PSEC  | PFRAME |";
    for (int i = 0; i < (buffer.length() - 4) / (int)sizeof(track_descriptor_format_2h); ++i) {
        QString s;
        s += QStringLiteral(u" %1 |").arg((int)tr[i].session_number, 6);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].adr, 6);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].control, 6);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].tno, 6);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].point, 6, 16);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].min, 6);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].sec, 6);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].frame, 6);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].zero, 6, 16);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].p_min, 6);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].p_sec, 6);
        s += QStringLiteral(u" %1 |").arg((int)tr[i].p_frame, 6);
        qDebug().nospace().noquote() << s;
    }

    Toc::Toc toc;

    int sessionLeadOutOffset = 0;
    for (unsigned int i = 0; i < (buffer.length() - 4) / (unsigned int)sizeof(track_descriptor_format_2h); ++i) {
        if (tr[i].adr == 1 && tr[i].point == 0xa0)
            toc.set(Toc::FirstTrackNumber, tr[i].p_min);

        if (tr[i].adr == 1 && tr[i].point == 0xa1)
            toc.set(Toc::LastTrackNumber, tr[i].p_min);

        if (tr[i].adr == 1 && tr[i].point <= 0x63) {
            Toc::Track track;

            track.set(Toc::FirstSector, Toc::MSF2Frames(tr[i].p_min, tr[i].p_sec, tr[i].p_frame));
            track.set(Toc::SessionNumber, tr[i].session_number);
            track.set(Toc::Format, tr[i].control & 0x4 ? Toc::DATA : Toc::AUDIO);
            track.set(Toc::CopyPermitted, tr[i].control & 0x2);
            track.set(Toc::PreEmphasis, tr[i].control & 0x1);

            // there might have been a change of the session
            if (toc.trackCount() > 0 && (toc.lastTrack().get(Toc::SessionNumber).toInt() == track.get(Toc::SessionNumber).toInt()))
                toc.lastTrack().set(Toc::LastSector, track.get(Toc::FirstSector).toInt() - 1);

            toc.appendTrack(track);
        } else if (tr[i].point == 0xa2) {
            if (toc.trackCount() > 0)
                toc.lastTrack().set(Toc::LastSector, sessionLeadOutOffset - 1);
            sessionLeadOutOffset = Toc::MSF2Frames(tr[i].p_min, tr[i].p_sec, tr[i].p_frame);
        }
    }

    // Set tracknumbers
    int tracknumber = toc.get(Toc::FirstTrackNumber).toInt();
    for (int t = 1; t <= toc.trackCount(); ++t)
        toc.track(t).set(Toc::TrackNumber, tracknumber++);

    toc.set(Toc::FirstSector, LEADIN_SECTOR_COUNT);
    toc.set(Toc::LastSector, sessionLeadOutOffset - 1);

    toc.lastTrack().set(Toc::LastSector, sessionLeadOutOffset - 1);

    ::close(deviceHandle);

    return toc;
}

}

}
