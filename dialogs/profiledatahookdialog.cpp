/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatahookdialog.h"

#include "dialogs/profiledatahookwizarddialog.h"

#include <QDialogButtonBox>
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

    ui.qlineedit_command->setText(command);
    connect(ui.qlineedit_command, &QLineEdit::textEdited, this, &ProfileDataHookDialog::trigger_changed);

    connect(ui.kpushbutton_wizard, &QAbstractButton::clicked, this, &ProfileDataHookDialog::command_wizard);
    ui.kpushbutton_wizard->setIcon(QIcon::fromTheme("tools-wizard"));

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

void ProfileDataHookDialog::command_wizard()
{
    ProfileDataHookWizardDialog dialog(ui.qlineedit_command->text(), this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    ui.qlineedit_command->setText(dialog.command);
    trigger_changed();
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
        applyButton->setEnabled(ui.qlineedit_command->text() != command);
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

    const bool success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HOOK_COMMAND_INDEX), ui.qlineedit_command->text());

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
