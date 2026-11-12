/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatahookdialog.h"

ProfileDataHookDialog::ProfileDataHookDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : ProfileDataSubDialog(profile_model, profile_row, new_profile_mode, i18n("Command Settings"), parent)
{
    ui.setupUi(contentWidget());

    bind(ui.schemeedit_command, PROFILE_MODEL_COLUMN_HOOK_COMMAND_INDEX);
}
