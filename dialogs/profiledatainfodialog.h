/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEDATAINFODIALOG_H
#define PROFILEDATAINFODIALOG_H

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
    QVBoxLayout *mainLayout;
    QPushButton *applyButton;

    ProfileModel *profile_model;
    int profile_row;
    bool new_profile_mode;

    bool save();

    Error error;

    TextViewDialog *help_dialog;
};

#endif
