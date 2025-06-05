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

#include "ui_profiledatalogfilewidgetUI.h"

namespace Audex
{

class ProfileDataLogFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDataLogFileDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent = nullptr);
    ~ProfileDataLogFileDialog() override;

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataLogFileWidgetUI ui;
    QPointer<QPushButton> applyButton;

    ProfileModel *profile_model;
    int profile_row;
    bool new_profile_mode;

    bool save();

    Error error;
};

}
