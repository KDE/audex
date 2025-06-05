/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "dialogs/errordialog.h"
#include "models/profilemodel.h"

#include "filenameschemewizarddialog.h"

#include "ui_profiledatacoverwidgetUI.h"

namespace Audex
{

class ProfileDataCoverDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataCoverDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent = nullptr);
    ~ProfileDataCoverDialog() override;

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();
    void enable_scale(bool enabled);

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataCoverWidgetUI ui;
    QPointer<QPushButton> applyButton;

    ProfileModel *profile_model;
    int profile_row;
    bool new_profile_mode;

    bool save();

    Error error;
};

}
