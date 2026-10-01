/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_devicewidgetUI.h"

#include "online/accuraterip.h"
#include "utils/devicesettings.h"
#include "utils/discsource.h"

#include <QFutureWatcher>
#include <QHash>
#include <QPointer>

#include <atomic>
#include <memory>

class DiscController;
class KMessageWidget;

class deviceWidgetUI : public QWidget, public Ui::DeviceWidgetUI
{
public:
    explicit deviceWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

// Device settings page: all settings are per drive. The combo box on top
// selects which drive is being edited; edits are stashed in memory when
// switching drives. KConfigXT does not know these values, so SettingsDialog
// asks this page for changes and calls loadSettings()/saveSettings().
class deviceWidget : public deviceWidgetUI
{
    Q_OBJECT
public:
    explicit deviceWidget(DiscController *discController, QWidget *parent = nullptr);
    ~deviceWidget() override;

    bool hasChanged() const; // edits not yet written, any drive
    bool isDefault() const; // drive on screen has the factory defaults

public Q_SLOTS:
    void loadSettings(); // (re)load from the config, discarding all edits
    void saveSettings(); // write the edited drives to the config
    void setDefaults(); // factory defaults for the drive on screen

Q_SIGNALS:
    void changed();

private Q_SLOTS:
    void deviceSwitched(int index);
    void drivesChanged();
    void fetchOffset(); // "Fetch from AccurateRip" button
    void offsetFetchFinished(const QList<Audex::AccurateRip::DriveOffset> &offsets);
    void offsetFetchFailed(const QString &message);
    void detectOffset(); // "Detect with inserted CD" button
    void offsetDetectionFinished();
    void runAssistant(); // "Test this drive..." button

private:
    void fillDriveList();
    void stashShownDrive();
    void showDrive(const QString &udi);
    DeviceSettings::Values widgetValues() const;
    void setWidgetValues(const DeviceSettings::Values &values);
    QString currentUdi() const;
    QString currentDriveName() const;
    Audex::DriveEntry currentDrive() const;

    void updateC2Availability();
    void updateFeatureSummary();
    static void applyFeatures(const Audex::Rip::DriveFeatures &features, DeviceSettings::Values *values);

    void updateAccuracyEnabling();
    bool m_c2Unsupported = false;

    QPointer<DiscController> m_discController;
    QHash<QString, DeviceSettings::Values> m_edited; // per drive, not yet applied
    QHash<QString, Audex::Rip::DriveFeatures> m_measured; // ... same for the assistant results
    QString m_shownUdi;
    Audex::AccurateRip::Client *m_arClient = nullptr;
    QString m_offsetDriveName; // drive the running offset fetch was started for
    KMessageWidget *m_fetchMessage = nullptr; // inline feedback for offset fetch and detection
    KMessageWidget *m_streamMessage = nullptr; // the drive test found no accurate stream
    QString m_c2ToolTip; // original tooltip from the .ui file
    QFutureWatcher<Audex::AccurateRip::OffsetDetection> m_detectWatcher;
    std::shared_ptr<std::atomic_bool> m_detectCancel;
    QString m_detectUdi; // drive the running detection was started for
};
