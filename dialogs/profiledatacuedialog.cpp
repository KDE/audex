/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatacuedialog.h"

ProfileDataCueDialog::ProfileDataCueDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : ProfileDataSubDialog(profile_model, profile_row, new_profile_mode, i18n("Cue Sheet Settings"), parent)
{
    ui.setupUi(contentWidget());

    bind(ui.schemeedit_scheme, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX);
    bind(ui.checkBox_mcn_isrc, PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX);
}
