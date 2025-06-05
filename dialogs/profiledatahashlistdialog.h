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

#include "filenameschemewizarddialog.h"

#include "dialogs/errordialog.h"
#include "models/profilemodel.h"

#include "ui_profiledatahashlistwidgetUI.h"

namespace Audex
{

class ProfileDataHashlistDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataHashlistDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent = nullptr);
    ~ProfileDataHashlistDialog() override;

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataHashlistWidgetUI ui;
    QPointer<QPushButton> applyButton;

    ProfileModel *profile_model;
    int profile_row;
    bool new_profile_mode;

    bool save();

    Error error;
};

}
