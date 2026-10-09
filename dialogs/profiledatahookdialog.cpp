/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatahookdialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

ProfileDataHookDialog::ProfileDataHookDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;

    if (!this->profile_model) {
        qWarning() << "ProfileDataHookDialog() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    const QString command = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HOOK_COMMAND_INDEX)).toString();

    setWindowTitle(i18n("Command Settings"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    if (!new_profile_mode)
        buttons |= QDialogButtonBox::Apply;

    QDialogButtonBox *buttonBox = new QDialogButtonBox(buttons);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    if (!new_profile_mode)
        applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataHookDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataHookDialog::reject);
    if (!new_profile_mode)
        connect(applyButton, &QPushButton::clicked, this, &ProfileDataHookDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    ui.schemeedit_command->setScheme(command);
    connect(ui.schemeedit_command, &SchemeEdit::edited, this, &ProfileDataHookDialog::trigger_changed);

    if (applyButton)
        applyButton->setEnabled(false);
}

void ProfileDataHookDialog::slotAccepted()
{
    if (save())
        accept();
    else
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataHookDialog::slotApplied()
{
    if (!save())
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataHookDialog::trigger_changed()
{
    if (!profile_model) {
        qWarning() << "ProfileDataHookDialog::trigger_changed() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    if (applyButton) {
        const QString command = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HOOK_COMMAND_INDEX)).toString();
        applyButton->setEnabled(ui.schemeedit_command->scheme() != command);
    }
}

bool ProfileDataHookDialog::save()
{
    if (!profile_model) {
        qWarning() << "ProfileDataHookDialog::save() called with null model pointers";
        Q_ASSERT(profile_model);
        return false;
    }

    error.clear();

    const bool success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HOOK_COMMAND_INDEX), ui.schemeedit_command->scheme());

    if (!success)
        error = profile_model->lastError();

    if (success) {
        profile_model->commit();
        if (applyButton)
            applyButton->setEnabled(false);
        return true;
    }

    return false;
}
