/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatacoverdialog.h"

ProfileDataCoverDialog::ProfileDataCoverDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : ProfileDataSubDialog(profile_model, profile_row, new_profile_mode, i18n("Cover Settings"), parent)
{
    ui.setupUi(contentWidget());

    bind(ui.checkBox_scale, PROFILE_MODEL_COLUMN_SC_SCALE_INDEX);
    enable_scale(ui.checkBox_scale->isChecked());
    connect(ui.checkBox_scale, &QAbstractButton::toggled, this, &ProfileDataCoverDialog::enable_scale);

    const QSize size = storedValue(PROFILE_MODEL_COLUMN_SC_SIZE_INDEX).toSize();
    ui.kintspinbox_x->setValue(size.width());
    ui.kintspinbox_y->setValue(size.height());
    connect(ui.kintspinbox_x, &QSpinBox::valueChanged, this, &ProfileDataCoverDialog::trigger_changed);
    connect(ui.kintspinbox_y, &QSpinBox::valueChanged, this, &ProfileDataCoverDialog::trigger_changed);
    bindField(PROFILE_MODEL_COLUMN_SC_SIZE_INDEX, [this] {
        return QSize(ui.kintspinbox_x->value(), ui.kintspinbox_y->value());
    });

    // anything else (legacy BMP etc.) means JPEG
    ui.checkBox_png->setChecked(storedValue(PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX).toString() == QStringLiteral("PNG"));
    connect(ui.checkBox_png, &QAbstractButton::toggled, this, &ProfileDataCoverDialog::trigger_changed);
    bindField(PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX, [this] {
        return ui.checkBox_png->isChecked() ? QStringLiteral("PNG") : QStringLiteral("JPEG");
    });

    bind(ui.schemeedit_scheme, PROFILE_MODEL_COLUMN_SC_NAME_INDEX);
}

void ProfileDataCoverDialog::enable_scale(bool enabled)
{
    ui.label_x->setEnabled(enabled);
    ui.kintspinbox_x->setEnabled(enabled);
    ui.kintspinbox_y->setEnabled(enabled);
}
