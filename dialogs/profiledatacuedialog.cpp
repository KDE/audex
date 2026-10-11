/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatacuedialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

ProfileDataCueDialog::ProfileDataCueDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    this->profile_model = profile_model;

    if (!this->profile_model) {
        qWarning() << "ProfileDataCueDialog() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    setWindowTitle(i18n("Cue Sheet Settings"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    if (!new_profile_mode)
        buttons |= QDialogButtonBox::Apply;

    auto *buttonBox = new QDialogButtonBox(buttons);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    if (!new_profile_mode)
        applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataCueDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataCueDialog::reject);
    if (!new_profile_mode)
        connect(applyButton, &QPushButton::clicked, this, &ProfileDataCueDialog::slotApplied);

    auto *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    ui.schemeedit_scheme->setScheme(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX)).toString());
    connect(ui.schemeedit_scheme, &SchemeEdit::edited, this, &ProfileDataCueDialog::trigger_changed);

    ui.checkBox_mcn_isrc->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX)).toBool());
    connect(ui.checkBox_mcn_isrc, &QAbstractButton::toggled, this, &ProfileDataCueDialog::trigger_changed);

    if (applyButton)
        applyButton->setEnabled(false);
}

void ProfileDataCueDialog::slotAccepted()
{
    if (save())
        accept();
    else
        ErrorDialog::show(this, error);
}

void ProfileDataCueDialog::slotApplied()
{
    if (!save())
        ErrorDialog::show(this, error);
}

void ProfileDataCueDialog::trigger_changed()
{
    if (!profile_model || !applyButton)
        return;
    applyButton->setEnabled(
        ui.schemeedit_scheme->scheme() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX)).toString()
        || ui.checkBox_mcn_isrc->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX)).toBool());
}

bool ProfileDataCueDialog::save()
{
    if (!profile_model) {
        qWarning() << "ProfileDataCueDialog::save() called with null model pointers";
        Q_ASSERT(profile_model);
        return false;
    }

    error.clear();
    if (!profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX), ui.schemeedit_scheme->scheme())
        || !profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX), ui.checkBox_mcn_isrc->isChecked())) {
        error = profile_model->lastError();
        return false;
    }
    profile_model->commit();

    if (applyButton)
        applyButton->setEnabled(false);
    return true;
}
