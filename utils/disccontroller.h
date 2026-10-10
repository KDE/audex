/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QFutureWatcher>
#include <QHash>
#include <QObject>
#include <QString>

#include <optional>

#include "utils/discsource.h"

// Watches the optical drives via Solid and reads an inserted audio disc with
// the new engine (in a worker thread). Multiple drives are supported: the
// drive list is exposed for the GUI (toolbar selector, device settings) and
// one drive is the current one. Emits discDetected() when TOC and CD-Text
// of the disc in the current drive are available and discRemoved() when that
// disc goes away.
class DiscController : public QObject
{
    Q_OBJECT

public:
    enum class Medium {
        None, // no disc (or one the system does not report, e.g. a blank one)
        Audio, // audio tracks, possibly with data (Enhanced CD)
        NoAudio // data disc
    };

    struct DriveInfo {
        QString udi; // Solid UDI of the drive (stable identifier)
        Audex::DriveEntry entry; // device node + display name
        Medium medium = Medium::None;
    };

    // what the current drive offers, for the main window
    enum class State {
        NoDrive,
        NoDisc,
        NoAudio,
        Reading,
        Failed, // see failureMessage(); retry() reads again
        Ready // discDetected() was emitted
    };

    explicit DiscController(QObject *parent = nullptr);
    ~DiscController() override;

    QList<DriveInfo> drives() const
    {
        return m_drives;
    }

    Audex::DriveEntry currentDrive() const
    {
        return m_drive;
    }
    QString currentDriveUdi() const
    {
        return m_driveUdi;
    }
    bool setCurrentDrive(const QString &udi); // returns false if the UDI is unknown

    State state() const
    {
        return m_state;
    }
    QString failureMessage() const
    {
        return m_failedMessage;
    }
    QString failureDetails() const
    {
        return m_failedDetails;
    }

    // C2 capability of a drive, once a disc was read this session
    std::optional<bool> driveSupportsC2(const QString &udi) const;

public Q_SLOTS:
    void eject();
    void rescan();
    void retry(); // reads the disc of the current drive again after a failure

Q_SIGNALS:
    void discDetected(const Audex::DiscReadResult &result);
    void discRemoved();
    void failed(const QString &message, const QString &details);
    void drivesChanged();
    void currentDriveChanged();
    void stateChanged();

private Q_SLOTS:
    void onDeviceAdded(const QString &udi);
    void onDeviceRemoved(const QString &udi);
    void onDiscRead();

private:
    QList<DriveInfo> scanDrives(); // drives + their volumes via Solid
    void adoptScan(const QList<DriveInfo> &scan); // update m_drives, emit drivesChanged()
    QString volumeUdiFor(const QString &driveUdi) const;
    void pickCurrentDrive(); // choose a sane current drive if none/invalid
    void setCurrentDriveInternal(const QString &udi);
    void startRead(const QString &udi);
    void updateState(); // emits stateChanged() if it changed

    QList<DriveInfo> m_drives;
    QHash<QString, QString> m_volumeByDrive; // drive UDI -> volume (disc) UDI
    QString m_udi; // UDI of the current audio disc (the Solid "volume")
    QString m_driveUdi; // UDI of the current drive (persists without a disc)
    QHash<QString, bool> m_c2CapableByDrive; // drive UDI -> C2 capability
    Audex::DriveEntry m_drive;
    QFutureWatcher<Audex::DiscReadResult> m_watcher;
    QString m_readingUdi;
    QString m_failedUdi; // reading failed; skip until the disc is removed or retry()
    QString m_failedMessage;
    QString m_failedDetails;
    State m_state = State::NoDrive;
};
