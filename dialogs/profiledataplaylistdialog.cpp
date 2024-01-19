/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledataplaylistdialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataPlaylistDialog::ProfileDataPlaylistDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;
    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    applyButton = nullptr;

    // profile data playlist data
    QString format = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_FORMAT_INDEX)).toString();
    QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_NAME_INDEX)).toString();
    bool abs_file_path = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX)).toBool();
    bool utf8 = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_UTF8_INDEX)).toBool();

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

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    connect(ui.kpushbutton_scheme, SIGNAL(clicked()), this, SLOT(scheme_wizard()));
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.kcombobox_format->addItem("M3U (Textbased Winamp Playlist)", "M3U");
    ui.kcombobox_format->addItem("PLS (Textbased Playlist)", "PLS");
    ui.kcombobox_format->addItem("XSPF (XML Shareable Playlist Format)", "XSPF");
    {
        int i = ui.kcombobox_format->findData(format);
        ui.kcombobox_format->setCurrentIndex(i);
    }
    enable_abs_file_path(!(format == "XSPF"));
    enable_utf8(!(format == "XSPF"));
    connect(ui.kcombobox_format, SIGNAL(currentIndexChanged(int)), this, SLOT(trigger_changed()));

    ui.qlineedit_scheme->setText(scheme);
    connect(ui.qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    ui.checkBox_abs_file_path->setChecked(abs_file_path);
    connect(ui.checkBox_abs_file_path, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

    ui.checkBox_utf8->setChecked(utf8);
    connect(ui.checkBox_utf8, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

    if (applyButton)
        applyButton->setEnabled(false);
}

ProfileDataPlaylistDialog::~ProfileDataPlaylistDialog()
{
}

void ProfileDataPlaylistDialog::slotAccepted()
{
    if (save())
        accept();
    else
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataPlaylistDialog::slotApplied()
{
    if (!save())
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataPlaylistDialog::scheme_wizard()
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

void ProfileDataPlaylistDialog::trigger_changed()
{
    enable_abs_file_path(ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != "XSPF");
    enable_utf8(ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != "XSPF");

    if (applyButton) {
        QString format = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_FORMAT_INDEX)).toString();
        QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_NAME_INDEX)).toString();
        bool abs_file_path = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX)).toBool();
        bool utf8 = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_UTF8_INDEX)).toBool();

        if (ui.checkBox_abs_file_path->isChecked() != abs_file_path) {
            applyButton->setEnabled(true);
            return;
        }
        if (ui.checkBox_utf8->isChecked() != utf8) {
            applyButton->setEnabled(true);
            return;
        }
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

void ProfileDataPlaylistDialog::enable_abs_file_path(bool enabled)
{
    ui.checkBox_abs_file_path->setEnabled(enabled);
}

void ProfileDataPlaylistDialog::enable_utf8(bool enabled)
{
    ui.checkBox_utf8->setEnabled(enabled);
}

bool ProfileDataPlaylistDialog::save()
{
    QString format = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString();
    QString scheme = ui.qlineedit_scheme->text();
    bool abs_file_path = ui.checkBox_abs_file_path->isChecked();
    bool utf8 = ui.checkBox_utf8->isChecked();

    error.clear();
    bool success = true;

    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_FORMAT_INDEX), format);
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_NAME_INDEX), scheme);
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX), abs_file_path);
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_UTF8_INDEX), utf8);

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
