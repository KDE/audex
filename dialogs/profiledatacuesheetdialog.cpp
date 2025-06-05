/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatacuesheetdialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Audex
{

ProfileDataCueSheetDialog::ProfileDataCueSheetDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;
    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    applyButton = nullptr;

    // profile data cue sheet data
    QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX)).toString();
    bool add_mcn_and_isrc = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_ADD_MCN_AND_ISRC_INDEX)).toBool();

    setWindowTitle(i18n("Cue Sheet Settings"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    if (!new_profile_mode)
        buttons |= QDialogButtonBox::Apply;

    auto *buttonBox = new QDialogButtonBox(buttons);
    auto *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    if (!new_profile_mode)
        applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataCueSheetDialog::slotAccepted);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataCueSheetDialog::reject);
    if (!new_profile_mode)
        QObject::connect(applyButton, &QPushButton::clicked, this, &ProfileDataCueSheetDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    QObject::connect(ui.kpushbutton_scheme, &QPushButton::clicked, this, &ProfileDataCueSheetDialog::scheme_wizard);
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.qlineedit_scheme->setText(scheme);
    QObject::connect(ui.qlineedit_scheme, &QLineEdit::textEdited, this, &ProfileDataCueSheetDialog::trigger_changed);

    ui.checkBox_add_mcn_and_isrc->setChecked(add_mcn_and_isrc);
    QObject::connect(ui.checkBox_add_mcn_and_isrc, &QCheckBox::toggled, this, &ProfileDataCueSheetDialog::trigger_changed);

    if (applyButton)
        applyButton->setEnabled(false);
}

ProfileDataCueSheetDialog::~ProfileDataCueSheetDialog()
{
}

void ProfileDataCueSheetDialog::slotAccepted()
{
    if (save())
        accept();
    else
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataCueSheetDialog::slotApplied()
{
    if (!save())
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataCueSheetDialog::scheme_wizard()
{
    FilenameSchemeWizardDialog *dialog = new FilenameSchemeWizardDialog(ui.qlineedit_scheme->text(), "cue", this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    ui.qlineedit_scheme->setText(dialog->scheme);

    delete dialog;

    trigger_changed();
}

void ProfileDataCueSheetDialog::trigger_changed()
{
    if (applyButton) {
        QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX)).toString();
        bool add_mcn_and_isrc = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_ADD_MCN_AND_ISRC_INDEX)).toBool();

        if (ui.qlineedit_scheme->text() != scheme) {
            applyButton->setEnabled(true);
            return;
        }
        if (ui.checkBox_add_mcn_and_isrc->isChecked() != add_mcn_and_isrc) {
            applyButton->setEnabled(true);
            return;
        }
        applyButton->setEnabled(false);
    }
}

bool ProfileDataCueSheetDialog::save()
{
    QString scheme = ui.qlineedit_scheme->text();
    bool add_mcn_and_isrc = ui.checkBox_add_mcn_and_isrc->isChecked();

    error.clear();
    bool success = true;

    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX), scheme);
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_ADD_MCN_AND_ISRC_INDEX), add_mcn_and_isrc);

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

}
