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
#include <QTextBrowser>
#include <QVBoxLayout>

#include "dialogs/errordialog.h"
#include "dialogs/textviewdialog.h"
#include "models/profilemodel.h"

#include "filenameschemewizarddialog.h"

#include "ui_profiledatainfowidgetUI.h"

namespace Audex
{

class ProfileDataInfoDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataInfoDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent = nullptr);
    ~ProfileDataInfoDialog() override;

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();

    void help();

    void load_text();
    void save_text();

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataInfoWidgetUI ui;
    QPointer<QVBoxLayout> mainLayout;
    QPointer<QPushButton> applyButton;

    ProfileModel *profile_model;
    int profile_row;
    bool new_profile_mode;

    bool save();

    Error error;

    QPointer<TextViewDialog> help_dialog;
};

}
