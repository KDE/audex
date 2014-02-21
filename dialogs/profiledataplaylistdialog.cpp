/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2014 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
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

ProfileDataPlaylistDialog::ProfileDataPlaylistDialog(const QString& format, const QString &pattern, const bool absFilePath, QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  this->format = format;
  this->pattern = pattern;
  this->absFilePath = absFilePath;

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  setCaption(i18n("Playlist Settings"));

  setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

  connect(ui.kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));
  ui.kpushbutton_pattern->setIcon(KIcon("tools-wizard"));

  ui.kcombobox_format->addItem("M3U (Textbased Winamp Playlist)", "M3U");
  ui.kcombobox_format->addItem("PLS (Textbased Playlist)", "PLS");
  ui.kcombobox_format->addItem("XSPF (XML Shareable Playlist Format)", "XSPF");
  {
    int i = ui.kcombobox_format->findData(format);
    ui.kcombobox_format->setCurrentIndex(i);
  }
  enable_abs_file_path(!(format == "XSPF"));
  connect(ui.kcombobox_format, SIGNAL(currentIndexChanged(int)), this, SLOT(trigger_changed()));
  
  ui.klineedit_pattern->setText(pattern);
  connect(ui.klineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  ui.checkBox_abs_file_path->setChecked(absFilePath);
  connect(ui.checkBox_abs_file_path, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

  enableButtonApply(FALSE);
  showButtonSeparator(true);

}

ProfileDataPlaylistDialog::~ProfileDataPlaylistDialog() {
}

void ProfileDataPlaylistDialog::slotButtonClicked(int button) {
  if (button == KDialog::Ok) {
    save();
    accept();
  } else if (button == KDialog::Apply) {
    save();
  } else {
    KDialog::slotButtonClicked(button);
  }
}

void ProfileDataPlaylistDialog::pattern_wizard() {

  QString suffix = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString().toLower();

  SimplePatternWizardDialog *dialog = new SimplePatternWizardDialog(ui.klineedit_pattern->text(), suffix, this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }

  ui.klineedit_pattern->setText(dialog->pattern);

  delete dialog;

  trigger_changed();

}

void ProfileDataPlaylistDialog::trigger_changed() {
  enable_abs_file_path(ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != "XSPF");
  if (ui.checkBox_abs_file_path->isChecked() != absFilePath) { enableButtonApply(TRUE); return; }
  if (ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != format) { enableButtonApply(TRUE); return; }
  if (ui.klineedit_pattern->text() != pattern) { enableButtonApply(TRUE); return; }
  enableButtonApply(FALSE);
}

void ProfileDataPlaylistDialog::enable_abs_file_path(bool enabled) {
  ui.checkBox_abs_file_path->setEnabled(enabled);
}

bool ProfileDataPlaylistDialog::save() {
  format = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString();
  pattern = ui.klineedit_pattern->text();
  absFilePath = ui.checkBox_abs_file_path->isChecked();
  enableButtonApply(FALSE);
  return TRUE;
}
