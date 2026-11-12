/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_profiledatacuewidgetUI.h"

#include "dialogs/profiledatasubdialog.h"

class ProfileDataCueDialog : public ProfileDataSubDialog
{
    Q_OBJECT

public:
    ProfileDataCueDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent = nullptr);

private:
    Ui::ProfileDataCueWidgetUI ui;
};
