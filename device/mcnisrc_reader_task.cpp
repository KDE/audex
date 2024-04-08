/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mcnisrc_reader_task.h"

namespace Audex
{

namespace Device
{

MCNISRCReaderTask::MCNISRCReaderTask(const QString &driveUDI)
    : Task(driveUDI)
{
    read_mcn = true;
}

void MCNISRCReaderTask::setTracknumbersForReadISRC(const TracknumberSet &tracknumbers)
{
    this->tracknumbers = tracknumbers;
}

TracknumberSet MCNISRCReaderTask::tracknumbersForReadISRC() const
{
    return tracknumbers;
}

void MCNISRCReaderTask::run()
{
    if (isInterruptionRequested())
        return;

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << block_device;

    Q_EMIT started(drive_udi);

    int deviceHandle = ::open(block_device.constData(), O_RDONLY | O_NONBLOCK);
    if (deviceHandle == -1) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to initialize device:" << block_device;
        last_error = Message(i18n("Failed to initialize drive %1.", QString::fromLatin1(block_device)), Message::CRITICAL);
        Q_EMIT log(drive_udi, last_error);
        Q_EMIT finished(drive_udi, false);
        return;
    }

    Metadata::Metadata numbers;

    SCSI::ErrorCode ec;

    if (read_mcn) {

        QByteArray buffer = SCSI::readSubchannel(deviceHandle, ec, 0x2, 0);

        if (ec) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to read mcn (subchannel info):" << block_device;
            last_error = Message(i18n("Failed to read MCN from disc in drive %1.", QString::fromLatin1(block_device)),
                                 Message::CRITICAL,
                                 ec.errorCode(),
                                 ec.senseKeyString());
            Q_EMIT log(drive_udi, last_error);
            ::close(deviceHandle);
            Q_EMIT finished(drive_udi, false);
            return;
        }

        if (buffer.size() == 24) {
            QByteArray mcn = buffer.mid(9, 13);
            if (!mcn.isEmpty() && !is_all_zero(mcn)) {
                numbers.set(Metadata::MCN, mcn);
            } else {
                qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "No MCN found:" << block_device;
            }
        } else {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Wrong buffer site while reading MCN:" << buffer.size() << block_device;
            last_error = Message(i18n("Failed to read MCN from disc in drive %1. Wrong buffer size (%2).", QString::fromLatin1(block_device), buffer.size()),
                                 Message::ERROR);
            Q_EMIT log(drive_udi, last_error);
        }

    }

    if (isInterruptionRequested()) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Interrupted." << block_device;
        Q_EMIT log(drive_udi, Message(i18n("Reading MCN/ISRC Interrupted."), Message::WARNING));
        ::close(deviceHandle);
        Q_EMIT finished(drive_udi, false);
        return;
    }

    QList<int> tracknumberlist = tracknumbers.values();
    std::sort(tracknumberlist.begin(), tracknumberlist.end());

    for (int i = 0; i < tracknumberlist.count(); ++i) {

        int t = tracknumberlist.at(i);

        ec.clear();
        QByteArray buffer = SCSI::readSubchannel(deviceHandle, ec, 0x3, t);

        if (ec) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Failed to read mcn (subchannel info):" << block_device;
            last_error = Message(i18n("Failed to read MCN from disc in drive %1.", QString::fromLatin1(block_device)),
                                 Message::CRITICAL,
                                 ec.errorCode(),
                                 ec.senseKeyString());
            Q_EMIT log(drive_udi, last_error);
            ::close(deviceHandle);
            Q_EMIT finished(drive_udi, false);
            return;
        }

        if (buffer.size() == 24) {
            QByteArray isrc = buffer.mid(9, 12);
            if (!isrc.isEmpty() && !is_all_zero(isrc)) {
                isrc.insert(2, QLatin1Char('-').toLatin1());
                isrc.insert(6, QLatin1Char('-').toLatin1());
                isrc.insert(9, QLatin1Char('-').toLatin1());
                numbers.track(t).set(Metadata::ISRC, isrc);
            } else {
                qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "No ISRC found for track:" << t << block_device;
            }
        } else {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Wrong buffer site while reading ISRC for track:" << t << buffer.size()
                     << block_device;
            Q_EMIT log(drive_udi, Message(i18n("Failed to read MCN from disc in drive %1. Wrong buffer size (%2) for track %3.",
                                             QString::fromLatin1(block_device),
                                             buffer.size(),
                                             t),
                                        Message::WARNING));
        }

        if (isInterruptionRequested()) {
            qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Interrupted." << block_device;
            Q_EMIT log(drive_udi, Message(i18n("Reading MCN/ISRC Interrupted."), Message::WARNING));
            ::close(deviceHandle);
            Q_EMIT finished(drive_udi, false);
            return;
        }
    }

    numbers.confirm();

    ::close(deviceHandle);

    Q_EMIT finished(drive_udi, true, numbers);
}

bool MCNISRCReaderTask::is_all_zero(const QByteArray &data)
{
    for (int i = 0; i < data.length(); ++i)
        if (data.at(i) != u'0')
            return false;
    return true;
}

}

}
