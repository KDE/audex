/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "disccontroller.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusVariant>
#include <QFile>
#include <QMap>
#include <QScopeGuard>
#include <QTimer>

#include <QtConcurrent>

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/DeviceNotifier>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>

using namespace Qt::StringLiterals;

namespace
{

// The device node of an optical drive without a disc: Solid hides the block
// device of an empty drive, UDisks2 keeps it. The UDI of a drive is its
// UDisks2 object path.
QString udisksDeviceNode(const QString &driveUdi)
{
    const QString service = u"org.freedesktop.UDisks2"_s;
    const QDBusConnection bus = QDBusConnection::systemBus();
    QDBusMessage list = QDBusMessage::createMethodCall(service, u"/org/freedesktop/UDisks2/Manager"_s, u"org.freedesktop.UDisks2.Manager"_s, u"GetBlockDevices"_s);
    list << QVariantMap();
    const QDBusReply<QList<QDBusObjectPath>> blocks = bus.call(list, QDBus::Block, 2000);
    if (!blocks.isValid())
        return {};

    for (const QDBusObjectPath &block : blocks.value()) {
        const auto property = [&](const QString &name) {
            QDBusMessage get = QDBusMessage::createMethodCall(service, block.path(), u"org.freedesktop.DBus.Properties"_s, u"Get"_s);
            get << u"org.freedesktop.UDisks2.Block"_s << name;
            const QDBusReply<QDBusVariant> reply = bus.call(get, QDBus::Block, 2000);
            return reply.isValid() ? reply.value().variant() : QVariant();
        };
        if (property(u"Drive"_s).value<QDBusObjectPath>().path() != driveUdi)
            continue;
        QByteArray node = property(u"Device"_s).toByteArray();
        while (node.endsWith('\0'))
            node.chop(1);
        return QFile::decodeName(node);
    }
    return {};
}

}

DiscController::DiscController(QObject *parent)
    : QObject(parent)
{
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceAdded, this, &DiscController::onDeviceAdded);
    connect(Solid::DeviceNotifier::instance(), &Solid::DeviceNotifier::deviceRemoved, this, &DiscController::onDeviceRemoved);
    connect(&m_watcher, &QFutureWatcher<Audex::DiscReadResult>::finished, this, &DiscController::onDiscRead);

    QTimer::singleShot(0, this, &DiscController::rescan);
}

DiscController::~DiscController()
{
    // no waiting: a read of a hung drive blocks until the kernel gives up,
    // and the read holds copies only (see driveThreadPool())
}

QList<DiscController::DriveInfo> DiscController::scanDrives()
{
    // pass 1: the drives (Solid "drive" devices, keyed by UDI for stability)
    QMap<QString, DriveInfo> drives; // QMap: stable order in the GUI
    const QList<Solid::Device> devices = Solid::Device::allDevices();
    for (const Solid::Device &device : devices) {
        if (!device.as<Solid::OpticalDrive>())
            continue;
        DriveInfo info;
        info.udi = device.udi();
        info.entry = Audex::DriveEntry{QString(), u"%1 %2"_s.arg(device.vendor(), device.product()).simplified()};
        drives.insert(info.udi, info);
    }

    // pass 2: the block devices below the drives bring the device node and,
    // with media inserted, the disc (audio content => rip-worthy)
    m_volumeByDrive.clear();
    for (const Solid::Device &device : devices) {
        const Solid::Block *block = device.as<Solid::Block>();
        if (!block || block->device().isEmpty())
            continue;
        auto it = drives.find(device.parentUdi());
        if (it == drives.end())
            continue;
        it->entry.id = block->device();
        if (it->entry.displayName.isEmpty())
            it->entry.displayName = block->device();
        const Solid::OpticalDisc *disc = device.as<Solid::OpticalDisc>();
        if (disc && (disc->availableContent() & Solid::OpticalDisc::Audio)) {
            it->medium = Medium::Audio;
            m_volumeByDrive.insert(it->udi, device.udi());
        } else if (disc) {
            it->medium = Medium::NoAudio;
        }
    }

    // pass 3: drives without a disc
    for (auto it = drives.begin(); it != drives.end(); ++it) {
        if (it->entry.id.isEmpty())
            it->entry.id = udisksDeviceNode(it->udi);
        if (it->entry.displayName.isEmpty())
            it->entry.displayName = it->entry.id;
    }

    QList<DriveInfo> result;
    for (auto it = drives.begin(); it != drives.end(); ++it)
        if (!it->entry.id.isEmpty()) // no device node -> unusable for us
            result.append(*it);
    return result;
}

void DiscController::adoptScan(const QList<DriveInfo> &scan)
{
    bool changed = scan.size() != m_drives.size();
    if (!changed) {
        for (int i = 0; i < scan.size(); ++i) {
            const DriveInfo &a = scan.at(i);
            const DriveInfo &b = m_drives.at(i);
            if (a.udi != b.udi || a.entry.id != b.entry.id || a.entry.displayName != b.entry.displayName || a.medium != b.medium) {
                changed = true;
                break;
            }
        }
    }
    if (changed) {
        m_drives = scan;
        Q_EMIT drivesChanged();
    }
}

QString DiscController::volumeUdiFor(const QString &driveUdi) const
{
    return m_volumeByDrive.value(driveUdi);
}

void DiscController::setCurrentDriveInternal(const QString &udi)
{
    if (udi == m_driveUdi)
        return;
    m_driveUdi = udi;
    m_drive = Audex::DriveEntry();
    for (const DriveInfo &d : m_drives) {
        if (d.udi == udi) {
            m_drive = d.entry;
            break;
        }
    }
    Q_EMIT currentDriveChanged();
}

void DiscController::pickCurrentDrive()
{
    for (const DriveInfo &d : m_drives) {
        if (d.udi == m_driveUdi) {
            m_drive = d.entry; // refresh node/name
            return; // current drive still present: keep it
        }
    }
    QString best;
    for (const DriveInfo &d : m_drives) {
        if (best.isEmpty())
            best = d.udi;
        if (d.medium == Medium::Audio) {
            best = d.udi;
            break;
        }
    }
    setCurrentDriveInternal(best);
}

bool DiscController::setCurrentDrive(const QString &udi)
{
    const auto state = qScopeGuard([this] {
        updateState();
    });
    if (udi == m_driveUdi)
        return true;
    for (const DriveInfo &d : m_drives) {
        if (d.udi != udi)
            continue;
        if (!m_udi.isEmpty()) {
            m_udi.clear();
            Q_EMIT discRemoved(); // the other drive's disc leaves the view
        }
        if (m_watcher.isRunning())
            m_readingUdi.clear(); // result will be discarded when it arrives
        setCurrentDriveInternal(udi);
        const QString volume = volumeUdiFor(udi);
        if (!volume.isEmpty() && volume != m_failedUdi)
            startRead(volume);
        return true;
    }
    return false;
}

std::optional<bool> DiscController::driveSupportsC2(const QString &udi) const
{
    const auto it = m_c2CapableByDrive.constFind(udi);
    if (it == m_c2CapableByDrive.cend())
        return std::nullopt;
    return it.value();
}

void DiscController::rescan()
{
    const auto state = qScopeGuard([this] {
        updateState();
    });
    adoptScan(scanDrives());
    pickCurrentDrive();
    if (m_udi.isEmpty() && m_readingUdi.isEmpty() && !m_driveUdi.isEmpty()) {
        const QString volume = volumeUdiFor(m_driveUdi);
        if (!volume.isEmpty() && volume != m_failedUdi)
            startRead(volume);
    }
}

void DiscController::onDeviceAdded(const QString &udi)
{
    Q_UNUSED(udi);
    const auto state = qScopeGuard([this] {
        updateState();
    });
    adoptScan(scanDrives());
    if (m_driveUdi.isEmpty())
        pickCurrentDrive();
    if (!m_udi.isEmpty() || !m_readingUdi.isEmpty())
        return;

    const QString volume = volumeUdiFor(m_driveUdi);
    if (!volume.isEmpty() && volume != m_failedUdi) {
        startRead(volume);
        return;
    }
    // the current drive has no (readable) disc: jump to a drive that has one
    for (const DriveInfo &d : m_drives) {
        if (d.udi != m_driveUdi && d.medium == Medium::Audio) {
            const QString other = volumeUdiFor(d.udi);
            if (!other.isEmpty() && other != m_failedUdi) {
                setCurrentDrive(d.udi);
                return;
            }
        }
    }
}

void DiscController::onDeviceRemoved(const QString &udi)
{
    const auto state = qScopeGuard([this] {
        updateState();
    });
    if (udi == m_readingUdi)
        m_readingUdi.clear(); // result is ignored when it arrives

    bool retry = false;
    if (udi == m_failedUdi) {
        m_failedUdi.clear();
        retry = true; // the failed disc is gone: a new attempt may be due
    }

    bool driveGone = false;
    for (const DriveInfo &d : m_drives) {
        if (d.udi == udi) {
            driveGone = true;
            Audex::clearDriveStalled(d.entry.id); // switched off or unplugged: it gets a new chance
        }
    }

    if (udi == m_udi) {
        m_udi.clear();
        Q_EMIT discRemoved(); // the current drive keeps its selection
    }

    adoptScan(scanDrives());

    if (driveGone && udi == m_driveUdi) {
        setCurrentDriveInternal(QString());
        pickCurrentDrive();
    }

    if (retry)
        QTimer::singleShot(0, this, &DiscController::rescan);
}

void DiscController::startRead(const QString &udi)
{
    const Solid::Device device(udi);
    const Solid::Block *block = device.as<Solid::Block>();
    if (!block || block->device().isEmpty())
        return;

    const Solid::Device drive = device.parent();
    QString name = u"%1 %2"_s.arg(drive.vendor(), drive.product()).simplified();
    if (name.isEmpty())
        name = block->device();

    m_readingUdi = udi;
    m_drive = Audex::DriveEntry{block->device(), name};
    m_driveUdi = drive.udi();

    const Audex::DriveEntry driveEntry = m_drive;
    m_watcher.setFuture(QtConcurrent::run(Audex::driveThreadPool(), [driveEntry] {
        return Audex::readDisc(driveEntry);
    }));
}

void DiscController::onDiscRead()
{
    const auto state = qScopeGuard([this] {
        updateState();
    });
    const QString udi = m_readingUdi;
    m_readingUdi.clear();

    const Audex::DiscReadResult result = m_watcher.result();
    if (udi.isEmpty())
        return; // the device went away or the drive was switched while reading

    if (!result.ok) {
        // do not hammer the drive with retries: wait until the disc is
        // removed (the UDI is the same for the next disc in this drive)
        m_failedUdi = udi;
        m_failedMessage = result.error;
        m_failedDetails = result.notes.join(u'\n');
        Q_EMIT failed(m_failedMessage, m_failedDetails);
        return;
    }

    m_udi = udi;
    Q_EMIT discDetected(result);
}

void DiscController::retry()
{
    const QString volume = volumeUdiFor(m_driveUdi);
    if (volume.isEmpty() || volume != m_failedUdi || !m_readingUdi.isEmpty())
        return;
    m_failedUdi.clear();
    startRead(volume);
    updateState();
}

void DiscController::updateState()
{
    State state = State::NoDrive;
    if (!m_driveUdi.isEmpty()) {
        Medium medium = Medium::None;
        for (const DriveInfo &d : std::as_const(m_drives))
            if (d.udi == m_driveUdi)
                medium = d.medium;
        const QString volume = volumeUdiFor(m_driveUdi);
        if (!m_udi.isEmpty())
            state = State::Ready;
        else if (!m_readingUdi.isEmpty())
            state = State::Reading;
        else if (!volume.isEmpty() && volume == m_failedUdi)
            state = State::Failed;
        else if (medium == Medium::Audio)
            state = State::Reading; // the read is about to start
        else
            state = medium == Medium::NoAudio ? State::NoAudio : State::NoDisc;
    }
    if (state != m_state) {
        m_state = state;
        Q_EMIT stateChanged();
    }
}

void DiscController::eject()
{
    if (m_driveUdi.isEmpty())
        return;
    Solid::Device device(m_driveUdi);
    if (auto *drive = device.as<Solid::OpticalDrive>())
        drive->eject();
}
