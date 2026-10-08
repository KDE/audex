/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_profiledatacoverwidgetUI.h"

#include "dialogs/errordialog.h"
#include "models/profilemodel.h"

#include <QDialog>
#include <QPointer>

class ProfileDataCoverDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataCoverDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent = nullptr);

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();
    void enable_scale(bool enabled);

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataCoverWidgetUI ui;
    QPushButton *applyButton = nullptr;

    QPointer<ProfileModel> profile_model;
    int profile_row;
    bool new_profile_mode;

    bool save();

    Error error;
};
