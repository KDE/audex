/*
 * AUDEX CDDA EXTRACTOR
 *
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "manager.h"

#include "tocreader.h"

namespace Audex
{

namespace Device
{

Manager::Manager(QObject *parent)
    : auto_read_toc(true)
{
    Q_UNUSED(parent);

    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &Manager::add_device);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &Manager::remove_device);

    connect(&cddb, &KCDDB::Client::finished, this, &Manager::cddb_lookup_done);
    connect(&cd_text, &CDTextReader::finished, this, &Manager::cdtext_lookup_done);
}

Manager::~Manager()
{
    clear();
}

const QStringList Manager::driveUDIList() const
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;
    QStringList result;
    for (auto &item : p_drives) {
        if (item) {
            result.append(item->device.udi());
        }
    }
    return result;
}

const QString Manager::driveVendor(const QString &driveUDI) const
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;
    if (auto drive = p_drives.value(driveUDI, nullptr))
        return drive->vendor;
    return QString();
}

const QString Manager::driveModel(const QString &driveUDI) const
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;
    if (auto drive = p_drives.value(driveUDI, nullptr))
        return drive->model;
    return QString();
}

const QByteArray Manager::blockDevice(const QString &driveUDI) const
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;
    if (auto drive = p_drives.value(driveUDI)) {
        if (drive->device.is<Solid::Block>())
            return drive->device.as<Solid::Block>()->device().toLatin1();
    }
    return QByteArray();
}

const QStringList Manager::discUDIList() const
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;
    QStringList result;
    for (auto &item : p_audio_discs) {
        if (item) {
            result.append(item->device.udi());
        }
    }
    return result;
}

const CDDA Manager::cdda(const QString &discUDI) const
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;
    if (auto disc = p_audio_discs.value(discUDI, nullptr)) {
        return disc->cdda;
    }
    return CDDA();
}

const QByteArray Manager::blockDeviceFromDisc(const QString &discUDI) const
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;
    if (auto disc = p_audio_discs.value(discUDI, nullptr)) {
        if (disc->device.is<Solid::Block>())
            return disc->device.as<Solid::Block>()->device().toLatin1();
    }
    return QByteArray();
}

const QString Manager::discPrettyName(const QString &discUDI) const
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;
    if (auto disc = p_audio_discs.value(discUDI, nullptr)) {
        return QString("%1 [%2]").arg(discUDI.section(QLatin1Char('/'), -1)).arg(disc->drive->model);
    }
    return QString();
}

void Manager::scanBus()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    p_drives.clear();
    p_audio_discs.clear();

    QList<Solid::Device> drives = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDrive);
    QList<Solid::Device> discs = Solid::Device::listFromType(Solid::DeviceInterface::OpticalDisc);

    for (int i = 0; i < drives.count(); ++i)
        add_drive(drives.value(i).udi());

    for (int i = 0; i < discs.count(); ++i)
        add_disc(discs.value(i).udi());
}

void Manager::readTOC(const QString &discUDI)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    QString error;
    p_audio_discs.value(discUDI)->cdda.toc() = Device::readTOC(discUDI, error);

    if (!error.isEmpty()) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << error;
    }
}

void Manager::eject(const QString &udi)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << udi;
    if (auto drive = p_drives.value(udi, nullptr))
        drive->device.as<Solid::OpticalDrive>()->eject();
}

void Manager::ejectDisc(const QString &discUDI)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << discUDI;
    if (auto disc = p_audio_discs.value(discUDI, nullptr))
        disc->drive->device.as<Solid::OpticalDrive>()->eject();
}

void Manager::lookupCDDB(const QString &discUDI, const bool blocking)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << discUDI;

    auto disc = p_audio_discs.value(discUDI);

    if (!disc) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "No Audio CD available" << discUDI;
        return;
    }

    cddb.config().reparse();
    cddb.setBlockingMode(blocking);
    cddb_discudi_cache = discUDI;

    cddb.lookup(disc->cdda.toc().discSignature());
}

void Manager::readCDText(const QString &discUDI)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << discUDI;

    cd_text.read(discUDI);
}

void Manager::setMetadata(const QString &discUDI, const Metadata::Metadata metadata)
{
    if (auto disc = p_audio_discs.value(discUDI, nullptr)) {
        disc->cdda.setMetadata(metadata);
    }
}

void Manager::setMetadataInCurrentDisc(const Metadata::Metadata metadata)
{
    if (!current_disc_udi.isEmpty())
        setMetadata(current_disc_udi, metadata);
}

void Manager::add_device(const QString &udi)
{
    Solid::Device device(udi);

    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << udi << device.product() << device.vendor();

    if (device.is<Solid::OpticalDrive>()) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << udi << "is optical drive";
        add_drive(udi);
    } else if (device.is<Solid::OpticalDisc>()) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << udi << "is optical disc";
        add_disc(udi);
    }
}

void Manager::remove_device(const QString &udi)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << udi;

    if (auto disc = p_audio_discs.value(udi, nullptr)) {
        p_audio_discs.remove(udi);
        Q_EMIT audioDiscRemoved(udi);
    } else if (auto drive = p_drives.value(udi, nullptr)) {
        p_drives.remove(udi);
        Q_EMIT driveRemoved(udi);
    }
}

void Manager::cddb_lookup_done(KCDDB::Result result)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << result;

    const QString disc_udi = cddb_discudi_cache;
    cddb_discudi_cache.clear();

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
        case KCDDB::NoRecordFound:;
        case KCDDB::MultipleRecordFound:;
        case KCDDB::Success:;
        default:
            qDebug() << "No data entry found in the CDDB database.";
        }
        Q_EMIT CDDBLookupDone(disc_udi, false);
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
            Q_EMIT CDDBLookupDone(disc_udi, false);
            return;
        }
    }

    Metadata::Metadata metadata;

    metadata.set(Metadata::Artist, info.get(KCDDB::Artist));
    metadata.set(Metadata::Album, info.get(KCDDB::Title));
    metadata.set(Metadata::CDDBCategory, info.get(KCDDB::Category));
    metadata.set(Metadata::Genre, info.get(KCDDB::Genre));

    QString comment = info.get(KCDDB::Comment).toString();
    QRegularExpression regex(QStringLiteral("YEAR:\\s*(\\d{4})\\s*"));
    QRegularExpressionMatch match = regex.match(comment);
    QString yearString;
    if (match.hasMatch()) {
        yearString = match.captured(1);
    }
    QString year = info.get(KCDDB::Year).toString();
    if (year.isEmpty())
        year = yearString;
    comment.remove(match.capturedStart(1) - 6, match.capturedEnd(1) - match.capturedStart(1) + 6);
    metadata.set(Metadata::Comment, comment.trimmed());
    metadata.set(Metadata::Year, year);

    for (int t = 1; t <= info.numberOfTracks(); ++t) {
        Metadata::Track track;
        track.set(Metadata::Artist, info.track(t - 1).get(KCDDB::Artist));
        track.set(Metadata::Title, info.track(t - 1).get(KCDDB::Title));
        track.set(Metadata::TrackNumber, t);
        metadata.appendTrack(track);
    }
    metadata.confirm();

    if (auto disc = p_audio_discs.value(disc_udi, nullptr))
        disc->cdda.setMetadata(metadata);

    Q_EMIT CDDBLookupDone(disc_udi, true, metadata);
}

void Manager::cdtext_lookup_done(const QString &discUDI, const bool successful, const Metadata::Metadata metadata)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << discUDI << successful;

    if (successful) {
        if (auto disc = p_audio_discs.value(discUDI, nullptr))
            disc->cdda.setMetadata(metadata);
    }

    Q_EMIT CDTextReadDone(discUDI, successful, metadata);
}

void Manager::add_drive(const QString &drive_udi)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << drive_udi;

    auto drive = QSharedPointer<OpticalDiscDrive>(new OpticalDiscDrive);

    Solid::Device device(drive_udi);
    drive->device = device;
    QString error;
    drive->vendor = device.vendor();
    drive->model = device.product();

    p_drives.insert(drive_udi, drive);

    Q_EMIT driveDetected(drive_udi, device.vendor(), device.product());
}

void Manager::add_disc(const QString &disc_udi)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << disc_udi;

    Solid::Device device(disc_udi);
    if (is_optical_audio_disc(device)) {
        qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << disc_udi << "is optical audio disc";
        auto disc = QSharedPointer<OpticalAudioDisc>(new OpticalAudioDisc);
        disc->device = device;
        disc->cdda = CDDA(disc_udi);
        disc->drive = p_drives.value(device.parentUdi());
        p_audio_discs.insert(disc_udi, disc);
        if (auto_read_toc) {
            readTOC(disc_udi);
        }
        Q_EMIT audioDiscDetected(device.parentUdi(), disc_udi, disc->cdda);
    }
}

bool Manager::is_optical_audio_disc(const Solid::Device &device) const
{
    if (device.is<Solid::OpticalDisc>())
        return (device.as<Solid::OpticalDisc>()->availableContent() & Solid::OpticalDisc::Audio);

    return false;
}

void Manager::clear()
{
    p_audio_discs.clear();
    p_drives.clear();
}

}

}
