/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEDATAPLAYLISTDIALOG_H
#define PROFILEDATAPLAYLISTDIALOG_H

#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "dialogs/errordialog.h"
#include "models/profilemodel.h"

#include "filenameschemewizarddialog.h"

#include "ui_profiledataplaylistwidgetUI.h"

class ProfileDataPlaylistDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataPlaylistDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent = nullptr);
    ~ProfileDataPlaylistDialog() override;

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();
    void enable_abs_file_path(bool enabled);
    void enable_utf8(bool enabled);

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataPlaylistWidgetUI ui;
    QPushButton *applyButton;

    ProfileModel *profile_model;
    int profile_row;
    bool new_profile_mode;

    bool save();

    Error error;
};

#endif
