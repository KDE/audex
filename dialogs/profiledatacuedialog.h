/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_profiledatacuewidgetUI.h"

#include "dialogs/errordialog.h"
#include "models/profilemodel.h"

#include <QDialog>
#include <QPointer>

class ProfileDataCueDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataCueDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent = nullptr);

private Q_SLOTS:
    void trigger_changed();
    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataCueWidgetUI ui;
    QPushButton *applyButton = nullptr;

    QPointer<ProfileModel> profile_model;
    int profile_row;
    bool new_profile_mode;

    bool save();

    Error error;
};
