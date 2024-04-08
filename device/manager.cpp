/*
 * AUDEX CDDA EXTRACTOR
 *
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "manager.h"

namespace Audex
{

namespace Device
{

Manager::Manager(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<Message>("Message");
    qRegisterMetaType<DriveInfo>("DriveInfo");
    qRegisterMetaType<Toc::Toc>("Toc::Toc");
    qRegisterMetaType<Metadata::Metadata>("Metadata::Metadata");
    qRegisterMetaType<Checksum32Map>("Checksum32Map");

    auto_read_toc = false;
    accuraterip_verfiy = true;

    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &Manager::add_device);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &Manager::remove_device);

    connect(&cddb, &KCDDB::Client::finished, this, &Manager::cddb_lookup_finished);

    connect(&acr_database, &AccurateRip::Database::lookupFinished, this, &Manager::accuraterip_database_lookup_finished);
}

Manager::~Manager()
{
    tasks_mutex.lock();
    for (auto i = tasks.begin(), end = tasks.end(); i != end; ++i) {
        Task *task = i.value();
        if (task && task->isRunning())
            task->terminate();
        if (task) {
            delete task;
            task = nullptr;
        }
    }
    tasks_mutex.unlock();

    for (auto i = drives.begin(), end = drives.end(); i != end; ++i) {
        Drive *drive = i.value();
        if (drive)
            delete drive;
    }

}

Message Manager::lastError() const
{
    return last_error;
}

void Manager::resetError()
{
    last_error.clear();
}

void Manager::enableAutoReadTOC(const bool enabled)
{
    auto_read_toc = enabled;
}

bool Manager::autoReadTOCEnabled() const
{
    return auto_read_toc;
}

void Manager::enableAccurateRipVerify(const bool enabled)
{
    accuraterip_verfiy = enabled;
}

bool Manager::accurateRipVerify() const
{
    return accuraterip_verfiy;
}

int Manager::driveCount() const
{
    return drives.count();
}

QList<QString> Manager::driveUDIS() const
{
    return drives.keys();
}

Drive *Manager::drive(const QString &udi) const
{
    return drives.value(udi);
}

DriveList Manager::driveList() const
{
    return drives.values();
}

CDInfo Manager::getCDInfoByDrive(const QString &driveUDI) const
{
    Drive *drive = drives.value(driveUDI);
    if (drive)
        return drives.value(driveUDI)->cdInfo();
    return CDInfo();
}

bool Manager::discInDriveAvailable(const QString &driveUDI) const
{
    Drive *drive = drives.value(driveUDI);
    if (drive)
        return drives.value(driveUDI)->discAvailable();
    return false;
}

void Manager::scanBus()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    drives.clear();

    QList<Solid::Device> drives = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDrive);
    QList<Solid::Device> discs = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDisc);

    for (int i = 0; i < drives.count(); ++i)
        add_drive(drives.value(i).udi());

    for (int i = 0; i < discs.count(); ++i)
        add_disc(discs.value(i).udi());
}

void Manager::eject(const QString &driveUDI) const
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << driveUDI;

    Solid::Device device(driveUDI);
    device.as<Solid::OpticalDrive>()->eject();
}

void Manager::startFetchDriveInfoTask(const QString &driveUDI, const bool blocking)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << driveUDI;

    if (!drives.contains(driveUDI))
        return;

    if (has_task(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Drive is busy" << driveUDI;
        return;
    }

    last_error.clear();

    DriveInfoFetcherTask *task = new DriveInfoFetcherTask(driveUDI);

    register_task(driveUDI, task);

    connect(task, &DriveInfoFetcherTask::log, this, &Manager::task_log);
    connect(task, &DriveInfoFetcherTask::finished, this, &Manager::fetch_drive_info_task_finished);
    connect(task, &QThread::finished, task, &QThread::deleteLater);

    task->start();
    Q_EMIT taskStarted(driveUDI);

    if (blocking)
        task->wait();
}

void Manager::startReadTOCTask(const QString &driveUDI, const bool blocking)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << driveUDI;

    if (!discInDriveAvailable(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "NO Audio CD available in drive" << driveUDI;
        return;
    }

    if (has_task(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Drive is busy" << driveUDI;
        return;
    }

    last_error.clear();

    TocReaderTask *task = new TocReaderTask(driveUDI);

    register_task(driveUDI, task);

    connect(task, &TocReaderTask::log, this, &Manager::task_log);
    connect(task, &TocReaderTask::finished, this, &Manager::read_toc_task_finished);
    connect(task, &QThread::finished, task, &QThread::deleteLater);

    task->start();
    Q_EMIT taskStarted(driveUDI);

    if (blocking)
        task->wait();
}

void Manager::startReadCDTextTask(const QString &driveUDI, const bool blocking)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << driveUDI;

    if (!discInDriveAvailable(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "NO Audio CD available in drive" << driveUDI;
        return;
    }

    if (has_task(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Drive is busy" << driveUDI;
        return;
    }

    last_error.clear();

    CDTextReaderTask *task = new CDTextReaderTask(driveUDI);

    register_task(driveUDI, task);

    connect(task, &CDTextReaderTask::log, this, &Manager::task_log);
    connect(task, &CDTextReaderTask::finished, this, &Manager::read_cdtext_task_finished);
    connect(task, &QThread::finished, task, &QThread::deleteLater);

    task->start();
    Q_EMIT taskStarted(driveUDI);

    if (blocking)
        task->wait();
}

void Manager::startReadMcnIsrcTask(const QString &driveUDI, const bool readMCN, const TracknumberSet &tracks, const bool blocking)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << driveUDI;

    if (!discInDriveAvailable(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "No Audio CD available in drive" << driveUDI;
        return;
    }

    if (has_task(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Drive is busy" << driveUDI;
        return;
    }

    last_error.clear();

    MCNISRCReaderTask *task = new MCNISRCReaderTask(driveUDI);
    task->enableReadMCN(readMCN);
    task->setTracknumbersForReadISRC(tracks);

    register_task(driveUDI, task);

    connect(task, &MCNISRCReaderTask::log, this, &Manager::task_log);
    connect(task, &MCNISRCReaderTask::finished, this, &Manager::read_mcnisrc_task_finished);
    connect(task, &QThread::finished, task, &QThread::deleteLater);

    task->start();
    Q_EMIT taskStarted(driveUDI);

    if (blocking)
        task->wait();
}

void Manager::startRipTask(const QString &driveUDI, const TracknumberSet &tracks)
{

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << driveUDI;

    if (!discInDriveAvailable(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "No Audio CD available in drive" << driveUDI;
        return;
    }

    if (has_task(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Drive is busy" << driveUDI;
        return;
    }

    last_error.clear();

    RipTask *task = new RipTask(drive(driveUDI),
                                6, /*sample offset*/
                                false, /*overread*/
                                2, /*retries on read error*/
                                true, /*skip on read error after num retries*/
                                false, /*(try to) skip on hardware error*/
                                3200, /*buffer size in sectors*/
                                false /*subchannel sync check*/,
                                true /*C2 error pointer check*/,
                                true /*CRC32 checksums into TOC*/,
                                accuraterip_verfiy /*ACR checksums into TOC*/);

    task->setTracknumbersToRip(tracks);

    register_task(driveUDI, task);

    connect(task, &RipTask::output, this, &Manager::rip_task_output);
    connect(task, &RipTask::progress, this, &Manager::rip_task_progress);
    connect(task, &RipTask::nextTrack, this, &Manager::rip_task_nexttrack);
    connect(task, &RipTask::log, this, &Manager::task_log);
    connect(task, &RipTask::finished, this, &Manager::rip_task_finished);
    connect(task, &QThread::finished, task, &QThread::deleteLater);

    acr_checksums_cache.clear();
    acr_checksums_v2_cache.clear();

    task->start();
    Q_EMIT taskStarted(driveUDI);

}

void Manager::stopTask(const QString &driveUDI)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << driveUDI;

    if (!has_task(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "No running thread found" << driveUDI;
        return;
    }

    tasks_mutex.lock();
    Task *task = qobject_cast<Task *>(tasks.value(driveUDI));
    tasks_mutex.unlock();

    if (task->isRunning())
        task->requestInterruption();
}

void Manager::lookupCDDB(const QString &driveUDI, const bool blocking)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << driveUDI;

    if (!discInDriveAvailable(driveUDI)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "No Audio CD available in drive" << driveUDI;
        return;
    }

    cddb.config().reparse();
    cddb.setBlockingMode(blocking);
    cddb_driveudi_cache = driveUDI;

    cddb.lookup(drives.value(driveUDI)->cdInfo().toc().trackOffsetList());
}

void Manager::verifyAccurateRip(const QString &driveUDI, const Checksum32Map checksums, const Checksum32Map checksumsV2)
{

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << driveUDI;

    drives[driveUDI]->driveInfoMutex()->lock();
    Toc::Toc toc = drives[driveUDI]->cdInfo().toc();
    drives[driveUDI]->driveInfoMutex()->unlock();
    QByteArray acrid = Audex::AccurateRip::Id(toc);

    acr_driveudi_cache = driveUDI;
    acr_checksums_cache = checksums;
    acr_checksums_v2_cache = checksumsV2;

    acr_database.lookup(acrid, toc.audioTrackCount());

}

void Manager::add_device(const QString &udi)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << udi;

    Solid::Device device(udi);

    if (device.is<Solid::OpticalDrive>())
        add_drive(udi);
    else if (device.is<Solid::OpticalDisc>())
        add_disc(udi);
}

void Manager::remove_device(const QString &udi)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << udi;

    if (is_drive_udi(udi)) {

        delete drives.value(udi);
        drives.remove(udi);
        Q_EMIT driveRemoved(udi);

    } else if (Drive *drive = get_drive_by_disc_udi(udi)) {

        drive->clearCDInfo();
        Q_EMIT audioDiscRemoved(drive->getUDI(), udi);
    }
}

void Manager::fetch_drive_info_task_finished(const QString &drive_udi, const bool successful, const DriveInfo &drive_info)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << successful;

    unregister_task(qobject_cast<Task *>(sender()));

    drives[drive_udi]->setDriveInfo(drive_info);

    Q_EMIT taskFinished(drive_udi);
    Q_EMIT fetchDriveInfoTaskFinished(drive_udi, successful, drive_info);
}

void Manager::read_toc_task_finished(const QString &drive_udi, const bool successful, const Toc::Toc &toc)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << successful;

    unregister_task(qobject_cast<Task *>(sender()));

    drives[drive_udi]->cdInfoMutex()->lock();
    drives[drive_udi]->cdInfo().setToc(toc);
    drives[drive_udi]->cdInfoMutex()->unlock();

    Q_EMIT taskFinished(drive_udi);
    Q_EMIT readTOCTaskFinished(drive_udi, successful, toc);
}

void Manager::read_cdtext_task_finished(const QString &drive_udi, const bool successful, const Metadata::Metadata &metadata)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << successful;

    unregister_task(qobject_cast<Task *>(sender()));

    drives[drive_udi]->cdInfoMutex()->lock();
    drives[drive_udi]->cdInfo().setMetadata(metadata);
    drives[drive_udi]->cdInfoMutex()->unlock();

    Q_EMIT taskFinished(drive_udi);
    Q_EMIT readCDTextTaskFinished(drive_udi, successful, metadata);
}

void Manager::read_mcnisrc_task_finished(const QString &drive_udi, const bool successful, const Metadata::Metadata &metadata)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << successful;

    unregister_task(qobject_cast<Task *>(sender()));

    drives[drive_udi]->cdInfoMutex()->lock();
    drives[drive_udi]->cdInfo().mergeMetadata(metadata);
    drives[drive_udi]->cdInfoMutex()->unlock();

    Q_EMIT taskFinished(drive_udi);
    Q_EMIT readMcnIsrcTaskFinished(drive_udi, successful, metadata);
}

void Manager::cddb_lookup_finished(KCDDB::Result result)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << result;

    QString drive_udi = cddb_driveudi_cache;
    cddb_driveudi_cache.clear();

    if ((result != KCDDB::Success) && (result != KCDDB::MultipleRecordFound)) {
        switch (result) {
        case KCDDB::ServerError:
            qDebug() << "There is an error with the CDDB server. Please wait or contact the administrator of the CDDB server.";
            break;
        case KCDDB::HostNotFound:
            qDebug() << "Cannot find the CDDB server. Check your network. Maybe the CDDB server is offline.";
            break;
        case KCDDB::NoResponse:
            qDebug() << "Please wait, maybe the server is busy, or contact the CDDB server administrator.";
            break;
        case KCDDB::InvalidCategory:
            break;
        case KCDDB::UnknownError:
            break;
        case KCDDB::NoRecordFound:
            ;
        case KCDDB::MultipleRecordFound:
            ;
        case KCDDB::Success:
            ;
        default:
            qDebug() << "No data entry found in the CDDB database.";
        }
        Q_EMIT CDDBLookupFinished(drive_udi, false);
        return;
    }

    KCDDB::CDInfo info = cddb.lookupResponse().constFirst();
    if (cddb.lookupResponse().count() > 1) {
        KCDDB::CDInfoList cddb_info_list = cddb.lookupResponse();
        KCDDB::CDInfoList::iterator it;
        QStringList list;
        for (it = cddb_info_list.begin(); it != cddb_info_list.end(); ++it)
            list.append(QStringLiteral(u"%1, %2, %3, %4")
                        .arg((it->get(KCDDB::Artist).toString()),
                             it->get(KCDDB::Title).toString(),
                             it->get(KCDDB::Genre).toString(),
                             it->get(KCDDB::Year).toString()));

        bool ok = false;
        // Uses a ComboBox, could use UseListViewForComboBoxItems if necessary
        QString res = QInputDialog::getItem(nullptr, i18n("Select CDDB Entry"), i18n("Select an entry:"), list, 0, false, &ok, {});

        if (ok) {
            // user selected an item and pressed OK
            info = cddb_info_list[list.indexOf(res)];
        } else {
            // user pressed cancel
            Q_EMIT CDDBLookupFinished(drive_udi, false);
            return;
        }
    }

    Metadata::Metadata metadata;

    metadata.set(Metadata::Artist, info.get(KCDDB::Artist));
    metadata.set(Metadata::Album, info.get(KCDDB::Title));
    metadata.set(Metadata::CDDBCategory, info.get(KCDDB::Category));
    metadata.set(Metadata::Year, info.get(KCDDB::Year));
    metadata.set(Metadata::Genre, info.get(KCDDB::Genre));
    metadata.set(Metadata::Comment, info.get(KCDDB::Comment));
    for (int t = 1; t <= info.numberOfTracks(); ++t) {
        Metadata::Track track;
        track.set(Metadata::Artist, info.track(t - 1).get(KCDDB::Artist));
        track.set(Metadata::Title, info.track(t - 1).get(KCDDB::Title));
        track.set(Metadata::TrackNumber, t);
        metadata.appendTrack(track);
    }
    metadata.confirm();

    drives[drive_udi]->cdInfoMutex()->lock();
    drives[drive_udi]->cdInfo().setMetadata(metadata);
    drives[drive_udi]->cdInfoMutex()->unlock();

    Q_EMIT CDDBLookupFinished(drive_udi, true, metadata);
}

void Manager::rip_task_output(const QString &drive_udi, const QByteArray &data)
{
    //QString drive_udi = qobject_cast<RipThread *>(sender())->udi();
    Q_EMIT ripTaskOutput(drive_udi, data);
}

void Manager::rip_task_progress(const QString &drive_udi, const int tracknumber, const qreal fractionCurrentTrack, const qreal fraction, const int currentSector, const int sectorsRead, const qreal currentSpeed)
{
    Q_EMIT ripTaskProgress(drive_udi, tracknumber, fractionCurrentTrack, fraction, currentSector, sectorsRead, currentSpeed);
}

void Manager::rip_task_nexttrack(const QString &drive_udi, const int prev_tracknumber, const int tracknumber)
{
    Q_EMIT ripTaskNextTrack(drive_udi, prev_tracknumber, tracknumber);
}

void Manager::rip_task_finished(const QString &drive_udi, const bool successful, const Checksum32Map &checksumsCRC32, const Checksum32Map &checksumsACR, const Checksum32Map &checksumsACRV2)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << successful;

    // QString drive_udi = qobject_cast<RipThread *>(sender())->udi();
    // Checksum32Map checksums_crc32 = qobject_cast<RipThread *>(sender())->checksumsCRC32();
    // Checksum32Map checksums_acr = qobject_cast<RipThread *>(sender())->checksumsACR();
    // Checksum32Map checksums_acr_v2 = qobject_cast<RipThread *>(sender())->checksumsACRV2();

    // write recent CRC32 checksums to TOC
    QList<int> tracknumbers = checksumsCRC32.keys();

    drives[drive_udi]->cdInfoMutex()->lock();
    for (int i = 0; i < tracknumbers.count(); ++i)
        drives[drive_udi]->cdInfo().toc().track(tracknumbers.at(i)).set(Toc::ChecksumCRC32, checksumsCRC32.value(tracknumbers.at(i)));
    drives[drive_udi]->cdInfoMutex()->unlock();

    unregister_task(qobject_cast<Task *>(sender()));

    Q_EMIT taskFinished(drive_udi);
    Q_EMIT ripTaskFinished(drive_udi, successful);

    if (accuraterip_verfiy)
        verifyAccurateRip(drive_udi, checksumsACR, checksumsACRV2);

}

void Manager::accuraterip_database_lookup_finished(const bool successful, const QByteArray &id, const AccurateRip::Records &records)
{

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << successful << id;

    QMap<int, quint32> checksums = acr_checksums_cache;
    acr_checksums_cache.clear();

    QMap<int, quint32> checksums_v2 = acr_checksums_v2_cache;
    acr_checksums_v2_cache.clear();

    QString drive_udi = acr_driveudi_cache;
    acr_driveudi_cache.clear();

    if (!successful) {
        Q_EMIT log(drive_udi, Message(i18n("No record found in AccurateRip database."), Message::ERROR));
        return;
    }

    QList<int> tracknumbers = checksums.keys();

    for (int i = 0; i < tracknumbers.count(); ++i) {

        int tracknumber = tracknumbers.at(i);

        quint32 checksum = checksums.value(tracknumber);
        quint32 checksum_v2 = checksums_v2.value(tracknumber);

        if (checksum == 0 && checksum_v2 == 0)
            continue;

        int confidence;
        if (AccurateRip::verify(records, checksum, tracknumber, &confidence)) {
            Q_EMIT log(drive_udi, Message(i18n("Match found [checksum %1] for track number %2. Accurate rip verified with confidence value %3.", QStringLiteral(u"%1").arg(checksum, 8, 16, QChar(u'0')), tracknumber, confidence)));
        } else if (AccurateRip::verify(records, checksum_v2, tracknumber, &confidence)) {
            Q_EMIT log(drive_udi, Message(i18n("Match found [checksum %1] for track %2. Accurate rip verified with confidence value %3.", QStringLiteral(u"%1").arg(checksum_v2, 8, 16, QChar(u'0')), tracknumber, confidence)));
        } else {
            Q_EMIT log(drive_udi, Message(i18n("NO match found [checksums %1 and %2] for track number %3. Accurate rip NOT verified.", QStringLiteral(u"%1").arg(checksum, 8, 16, QChar(u'0')), QStringLiteral(u"%1").arg(checksum_v2, 8, 16, QChar(u'0')), tracknumber), Message::WARNING));
        }

    }

}

void Manager::task_log(const QString &drive_udi, const Message &msg)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << msg;

    //QString driveUDI = qobject_cast<Reader *>(sender())->udi();

    if (msg.type() == Message::CRITICAL || msg.type() == Message::ERROR) {
        last_error = msg;
    }
    Q_EMIT this->log(drive_udi, msg);
}

bool Manager::has_task(const QString &drive_udi)
{
    tasks_mutex.lock();
    bool result = tasks.contains(drive_udi);
    tasks_mutex.unlock();
    return result;
}

void Manager::register_task(const QString &drive_udi, Task *task)
{
    tasks_mutex.lock();
    tasks.insert(drive_udi, task);
    tasks_mutex.unlock();
}

void Manager::unregister_task(Task *task)
{
    tasks_mutex.lock();
    QList<QString> key_list = tasks.keys(task);
    for (int i = 0; i < key_list.count(); ++i)
        tasks.remove(key_list.at(i));
    tasks_mutex.unlock();
}

bool Manager::is_drive_udi(const QString &udi) const
{
    return drives.contains(udi);
}

Drive *Manager::get_drive_by_disc_udi(const QString &disc_udi) const
{
    for (auto it = drives.begin(); it != drives.end(); ++it)
        if (it.value()->discAvailable() && it.value()->cdInfo().udi() == disc_udi)
            return it.value();
    return nullptr;
}

bool Manager::is_disc_udi(const QString &udi) const
{
    return get_drive_by_disc_udi(udi) != nullptr;
}

void Manager::add_drive(const QString &drive_udi)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << drive_udi;

    if (drives.contains(drive_udi))
        return;

    Drive *drive = new Drive(drive_udi);

    drives.insert(drive_udi, drive);

    Q_EMIT driveDetected(drive_udi, drive);

    // TODO: It might be better to queue this, than this single shot hack
    QTimer::singleShot(500, [ = ]() {
        startFetchDriveInfoTask(drive_udi);
    });
}

void Manager::add_disc(const QString &disc_udi)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << disc_udi;

    if (discInDriveAvailable(disc_udi))
        return;

    Solid::Device device(disc_udi);

    if (device.as<Solid::OpticalDisc>()->availableContent() & Solid::OpticalDisc::Audio) {

        Drive *drive = drives.value(device.parentUdi());
        drive->setCDInfo(CDInfo(disc_udi));

        Q_EMIT audioDiscDetected(device.parentUdi(), disc_udi);

        if (auto_read_toc) {
            // TODO: It might be better to queue this, than this single shot hack
            QTimer::singleShot(1500, [ = ]() {
                startReadTOCTask(device.parentUdi());
            });
        }
    }
}

}

}
