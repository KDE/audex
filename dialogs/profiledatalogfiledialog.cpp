/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatalogfiledialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Audex
{

ProfileDataLogFileDialog::ProfileDataLogFileDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;
    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    applyButton = nullptr;

    setWindowTitle(i18n("Log Files Settings"));

    // profile data logfile data
    QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_NAME_INDEX)).toString();
    bool write_timestamps = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_WRITE_TIMESTAMPS_INDEX)).toBool();

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataLogFileDialog::slotAccepted);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataLogFileDialog::reject);
    QObject::connect(applyButton, &QPushButton::clicked, this, &ProfileDataLogFileDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    QObject::connect(ui.kpushbutton_scheme, SIGNAL(clicked()), this, SLOT(scheme_wizard()));
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.qlineedit_scheme->setText(scheme);
    QObject::connect(ui.qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    ui.checkBox_timestamps->setChecked(write_timestamps);
    QObject::connect(ui.checkBox_timestamps, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

    if (applyButton)
        applyButton->setEnabled(false);
}

ProfileDataLogFileDialog::~ProfileDataLogFileDialog()
{
}

void ProfileDataLogFileDialog::slotAccepted()
{
    if (save())
        accept();
    else
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataLogFileDialog::slotApplied()
{
    if (!save())
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataLogFileDialog::scheme_wizard()
{
    FilenameSchemeWizardDialog *dialog = new FilenameSchemeWizardDialog(ui.qlineedit_scheme->text(), "log", this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    ui.qlineedit_scheme->setText(dialog->scheme);

    delete dialog;

    trigger_changed();
}

void ProfileDataLogFileDialog::trigger_changed()
{
    if (applyButton) {
        QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_NAME_INDEX)).toString();
        bool write_timestamps = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_WRITE_TIMESTAMPS_INDEX)).toBool();

        if (ui.qlineedit_scheme->text() != scheme) {
            applyButton->setEnabled(true);
            return;
        }
        if (ui.checkBox_timestamps->isChecked() != write_timestamps) {
            applyButton->setEnabled(true);
            return;
        }
        applyButton->setEnabled(false);
    }
}

bool ProfileDataLogFileDialog::save()
{
    QString scheme = ui.qlineedit_scheme->text();
    bool write_timestamps = ui.checkBox_timestamps->isChecked();

    error.clear();
    bool success = true;

    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_NAME_INDEX), scheme);
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_WRITE_TIMESTAMPS_INDEX), write_timestamps);

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
