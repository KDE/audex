/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledataplaylistdialog.h"

ProfileDataPlaylistDialog::ProfileDataPlaylistDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : ProfileDataSubDialog(profile_model, profile_row, new_profile_mode, i18n("Playlist Settings"), parent)
{
    ui.setupUi(contentWidget());

    bind(ui.schemeedit_scheme, PROFILE_MODEL_COLUMN_PL_NAME_INDEX);
    bind(ui.checkBox_abs_file_path, PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX);
    bind(ui.checkBox_utf8, PROFILE_MODEL_COLUMN_PL_UTF8_INDEX);
}
