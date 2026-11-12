/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_profiledataplaylistwidgetUI.h"

#include "dialogs/profiledatasubdialog.h"

class ProfileDataPlaylistDialog : public ProfileDataSubDialog
{
    Q_OBJECT

public:
    ProfileDataPlaylistDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent = nullptr);

private:
    Ui::ProfileDataPlaylistWidgetUI ui;
};
