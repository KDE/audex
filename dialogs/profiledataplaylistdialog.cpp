/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledataplaylistdialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataPlaylistDialog::ProfileDataPlaylistDialog(const QString &format, const QString &scheme, const bool absFilePath, const bool utf8, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->format = format;
    this->scheme = scheme;
    this->absFilePath = absFilePath;
    this->utf8 = utf8;

    setWindowTitle(i18n("Playlist Settings"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataPlaylistDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataPlaylistDialog::reject);
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

    ui.checkBox_abs_file_path->setChecked(absFilePath);
    connect(ui.checkBox_abs_file_path, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

    ui.checkBox_utf8->setChecked(utf8);
    connect(ui.checkBox_utf8, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

    applyButton->setEnabled(false);
}

ProfileDataPlaylistDialog::~ProfileDataPlaylistDialog()
{
}

void ProfileDataPlaylistDialog::slotAccepted()
{
    save();
    accept();
}

void ProfileDataPlaylistDialog::slotApplied()
{
    save();
}

void ProfileDataPlaylistDialog::scheme_wizard()
{
    QString suffix = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString().toLower();

    SimpleSchemeWizardDialog *dialog = new SimpleSchemeWizardDialog(ui.qlineedit_scheme->text(), suffix, this);

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
    if (ui.checkBox_abs_file_path->isChecked() != absFilePath) {
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
    format = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString();
    scheme = ui.qlineedit_scheme->text();
    absFilePath = ui.checkBox_abs_file_path->isChecked();
    utf8 = ui.checkBox_utf8->isChecked();
    applyButton->setEnabled(false);
    return true;
}
