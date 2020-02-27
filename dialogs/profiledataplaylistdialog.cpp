/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "profiledataplaylistdialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataPlaylistDialog::ProfileDataPlaylistDialog(const QString &format, const QString &pattern, const bool absFilePath, const bool utf8, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->format = format;
    this->pattern = pattern;
    this->absFilePath = absFilePath;
    this->utf8 = utf8;

    setWindowTitle(i18n("Playlist Settings"));

    QVBoxLayout *mainLayout = new QVBoxLayout;
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

    connect(ui.kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));
    ui.kpushbutton_pattern->setIcon(QIcon::fromTheme("tools-wizard"));

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

    ui.qlineedit_pattern->setText(pattern);
    connect(ui.qlineedit_pattern, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

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

void ProfileDataPlaylistDialog::pattern_wizard()
{
    QString suffix = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString().toLower();

    SimplePatternWizardDialog *dialog = new SimplePatternWizardDialog(ui.qlineedit_pattern->text(), suffix, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    ui.qlineedit_pattern->setText(dialog->pattern);

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
    if (ui.qlineedit_pattern->text() != pattern) {
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
    pattern = ui.qlineedit_pattern->text();
    absFilePath = ui.checkBox_abs_file_path->isChecked();
    utf8 = ui.checkBox_utf8->isChecked();
    applyButton->setEnabled(false);
    return true;
}
