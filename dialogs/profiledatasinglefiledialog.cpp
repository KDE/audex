/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatasinglefiledialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Audex
{

ProfileDataSingleFileDialog::ProfileDataSingleFileDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;
    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    applyButton = nullptr;

    setWindowTitle(i18n("Single File Settings"));

    // profile data single file data
    QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SF_NAME_INDEX)).toString();

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    auto *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataSingleFileDialog::slotAccepted);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataSingleFileDialog::reject);
    QObject::connect(applyButton, &QPushButton::clicked, this, &ProfileDataSingleFileDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    QObject::connect(ui.kpushbutton_scheme, SIGNAL(clicked()), this, SLOT(scheme_wizard()));
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.qlineedit_scheme->setText(scheme);
    QObject::connect(ui.qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    if (applyButton)
        applyButton->setEnabled(false);
}

ProfileDataSingleFileDialog::~ProfileDataSingleFileDialog()
{
}

void ProfileDataSingleFileDialog::slotAccepted()
{
    if (save())
        accept();
    else
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataSingleFileDialog::slotApplied()
{
    if (!save())
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataSingleFileDialog::scheme_wizard()
{
    FilenameSchemeWizardDialog *dialog = new FilenameSchemeWizardDialog(ui.qlineedit_scheme->text(), "wav", this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    ui.qlineedit_scheme->setText(dialog->scheme);

    delete dialog;

    trigger_changed();
}

void ProfileDataSingleFileDialog::trigger_changed()
{
    if (applyButton) {
        QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SF_NAME_INDEX)).toString();
        if (ui.qlineedit_scheme->text() != scheme) {
            applyButton->setEnabled(true);
            return;
        }
        applyButton->setEnabled(false);
    }
}

bool ProfileDataSingleFileDialog::save()
{
    QString scheme = ui.qlineedit_scheme->text();

    error.clear();
    bool success = true;

    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SF_NAME_INDEX), scheme);

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
