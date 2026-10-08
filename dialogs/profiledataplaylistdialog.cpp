/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledataplaylistdialog.h"

#include "dialogs/filenameschemewizarddialog.h"

#include <KMessageBox>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

ProfileDataPlaylistDialog::ProfileDataPlaylistDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;

    if (!this->profile_model) {
        qWarning() << "ProfileDataPlaylistDialog() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    // profile data playlist data
    const QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_NAME_INDEX)).toString();
    const bool abs_file_path = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX)).toBool();
    const bool utf8 = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_UTF8_INDEX)).toBool();

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
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataPlaylistDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataPlaylistDialog::reject);
    if (!new_profile_mode)
        connect(applyButton, &QPushButton::clicked, this, &ProfileDataPlaylistDialog::slotApplied);

    auto *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    connect(ui.kpushbutton_scheme, &QAbstractButton::clicked, this, &ProfileDataPlaylistDialog::scheme_wizard);
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.qlineedit_scheme->setText(scheme);
    connect(ui.qlineedit_scheme, &QLineEdit::textEdited, this, &ProfileDataPlaylistDialog::trigger_changed);

    ui.checkBox_abs_file_path->setChecked(abs_file_path);
    connect(ui.checkBox_abs_file_path, &QAbstractButton::toggled, this, &ProfileDataPlaylistDialog::trigger_changed);

    ui.checkBox_utf8->setChecked(utf8);
    connect(ui.checkBox_utf8, &QAbstractButton::toggled, this, &ProfileDataPlaylistDialog::trigger_changed);

    if (applyButton)
        applyButton->setEnabled(false);
}

void ProfileDataPlaylistDialog::slotAccepted()
{
    if (save())
        accept();
}

void ProfileDataPlaylistDialog::slotApplied()
{
    save();
}

void ProfileDataPlaylistDialog::scheme_wizard()
{
    FilenameSchemeWizardDialog dialog(ui.qlineedit_scheme->text(), "m3u", this);
    if (dialog.exec() == QDialog::Accepted) {
        ui.qlineedit_scheme->setText(dialog.scheme);
        trigger_changed();
    }
}

void ProfileDataPlaylistDialog::trigger_changed()
{
    if (!profile_model) {
        qWarning() << "ProfileDataPlaylistDialog::trigger_changed() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    if (applyButton) {
        const QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_NAME_INDEX)).toString();
        const bool abs_file_path = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX)).toBool();
        const bool utf8 = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_UTF8_INDEX)).toBool();

        if (ui.checkBox_abs_file_path->isChecked() != abs_file_path) {
            applyButton->setEnabled(true);
            return;
        }

        if (ui.checkBox_utf8->isChecked() != utf8) {
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

bool ProfileDataPlaylistDialog::save()
{
    if (!profile_model) {
        qWarning() << "ProfileDataPlaylistDialog::save() called with null model pointers";
        Q_ASSERT(profile_model);
        return false;
    }

    const QString scheme = ui.qlineedit_scheme->text();
    const bool abs_file_path = ui.checkBox_abs_file_path->isChecked();
    const bool utf8 = ui.checkBox_utf8->isChecked();

    bool success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_NAME_INDEX), scheme);
    if (!success) {
        KMessageBox::error(this, i18n("Playlist scheme could not be saved."), i18n("Playlist Settings"));
        return false;
    }

    success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX), abs_file_path);
    if (!success) {
        KMessageBox::error(this, i18n("Playlist absolute file path setting could not be saved."), i18n("Playlist Settings"));
        return false;
    }

    success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_UTF8_INDEX), utf8);
    if (!success) {
        KMessageBox::error(this, i18n("Playlist UTF-8 setting could not be saved."), i18n("Playlist Settings"));
        return false;
    }

    if (applyButton)
        applyButton->setEnabled(false);
    return true;
}
