/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatahashlistdialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataHashlistDialog::ProfileDataHashlistDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;
    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    applyButton = nullptr;

    // profile data hashlist data
    QString format = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_FORMAT_INDEX)).toString();
    QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_NAME_INDEX)).toString();

    setWindowTitle(i18n("Playlist Settings"));

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
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataHashlistDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataHashlistDialog::reject);
    if (!new_profile_mode)
        connect(applyButton, &QPushButton::clicked, this, &ProfileDataHashlistDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    connect(ui.kpushbutton_scheme, SIGNAL(clicked()), this, SLOT(scheme_wizard()));
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.kcombobox_format->addItem(i18n("SFV (Simple File Verification)"), "SFV");
    ui.kcombobox_format->addItem(i18n("MD5 (Message-Digest Algorithm 5)"), "MD5");
    ui.kcombobox_format->addItem(i18n("SHA-256 (Secure Hash Algorithm)"), "SHA-256");
    {
        int i = ui.kcombobox_format->findData(format);
        ui.kcombobox_format->setCurrentIndex(i);
    }
    connect(ui.kcombobox_format, SIGNAL(currentIndexChanged(int)), this, SLOT(trigger_changed()));

    ui.qlineedit_scheme->setText(scheme);
    connect(ui.qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    if (applyButton)
        applyButton->setEnabled(false);
}

ProfileDataHashlistDialog::~ProfileDataHashlistDialog()
{
}

void ProfileDataHashlistDialog::slotAccepted()
{
    if (save())
        accept();
    else
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataHashlistDialog::slotApplied()
{
    if (!save())
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataHashlistDialog::scheme_wizard()
{
    QString suffix = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString().toLower();

    FilenameSchemeWizardDialog *dialog = new FilenameSchemeWizardDialog(ui.qlineedit_scheme->text(), suffix, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    ui.qlineedit_scheme->setText(dialog->scheme);

    delete dialog;

    trigger_changed();
}

void ProfileDataHashlistDialog::trigger_changed()
{
    if (applyButton) {
        QString format = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_FORMAT_INDEX)).toString();
        QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_NAME_INDEX)).toString();

        if (ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != format) {
            applyButton->setEnabled(true);
            return;
        }
        if (ui.qlineedit_scheme->text() != scheme) {
            applyButton->setEnabled(true);
            return;
        }
        applyButton->setEnabled(false);
    }
}

bool ProfileDataHashlistDialog::save()
{
    QString format = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString();
    QString scheme = ui.qlineedit_scheme->text();

    error.clear();
    bool success = true;

    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_FORMAT_INDEX), format);
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_NAME_INDEX), scheme);

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
