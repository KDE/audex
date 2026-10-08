/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_driveassistantwidgetUI.h"

#include "core/drivefeatures.h"
#include "utils/discsource.h"

#include <QDialog>
#include <QFutureWatcher>
#include <QPushButton>

#include <atomic>
#include <memory>

// Measures what one drive can do (core/drivefeatures.h) and, if asked for, its
// read offset. The dialog only reports; the device settings page decides what
// to do with the result.
class DriveAssistantDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DriveAssistantDialog(const Audex::DriveEntry &drive, QWidget *parent = nullptr);
    ~DriveAssistantDialog() override; // stops a running test and waits for it

    // valid once the dialog was accepted
    Audex::Rip::DriveFeatures features() const;
    bool offsetFound() const;
    int offset() const;

private Q_SLOTS:
    void startOrStop();
    void testFinished();

private:
    struct Result {
        QString error; // the test could not be run at all
        Audex::Rip::DriveFeatures features;
        bool offsetFound = false;
        int offset = 0;
    };

    void checkStarted(int index, int total, const Audex::Rip::FeatureCheck &check);
    void checkFinished(const Audex::Rip::FeatureCheck &check);
    void setRunning(bool running);

    Ui::DriveAssistantWidgetUI ui;
    Audex::DriveEntry m_drive;
    int m_steps = 0; // checks this run, including the read offset
    QPushButton *m_startButton = nullptr;
    QPushButton *m_applyButton = nullptr;
    QFutureWatcher<Result> m_watcher;
    std::shared_ptr<std::atomic_bool> m_cancel;
    Result m_result;
};
