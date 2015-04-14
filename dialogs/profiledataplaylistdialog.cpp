/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <http://userbase.kde.org/Audex>
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

ProfileDataPlaylistDialog::ProfileDataPlaylistDialog(const QString& format, const QString &pattern, const bool absFilePath, const bool utf8, QWidget *parent) : KDialog(parent)
{

  Q_UNUSED(parent);

  this->format = format;
  this->pattern = pattern;
  this->absFilePath = absFilePath;
  this->utf8 = utf8;

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
  enable_utf8(!(format == "XSPF"));
  connect(ui.kcombobox_format, SIGNAL(currentIndexChanged(int)), this, SLOT(trigger_changed()));

  ui.klineedit_pattern->setText(pattern);
  connect(ui.klineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  ui.checkBox_abs_file_path->setChecked(absFilePath);
  connect(ui.checkBox_abs_file_path, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

  ui.checkBox_utf8->setChecked(utf8);
  connect(ui.checkBox_utf8, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

  enableButtonApply(false);
  showButtonSeparator(true);

}

ProfileDataPlaylistDialog::~ProfileDataPlaylistDialog()
{
}

void ProfileDataPlaylistDialog::slotButtonClicked(int button)
{
  if (button == KDialog::Ok)
  {
    save();
    accept();
  }
  else if (button == KDialog::Apply)
  {
    save();
  }
  else
  {
    KDialog::slotButtonClicked(button);
  }
}

void ProfileDataPlaylistDialog::pattern_wizard()
{

  QString suffix = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString().toLower();

  SimplePatternWizardDialog *dialog = new SimplePatternWizardDialog(ui.klineedit_pattern->text(), suffix, this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }

  ui.klineedit_pattern->setText(dialog->pattern);

  delete dialog;

  trigger_changed();

}

void ProfileDataPlaylistDialog::trigger_changed()
{
  enable_abs_file_path(ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != "XSPF");
  enable_utf8(ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != "XSPF");
  if (ui.checkBox_abs_file_path->isChecked() != absFilePath) { enableButtonApply(true); return; }
  if (ui.checkBox_utf8->isChecked() != utf8) { enableButtonApply(true); return; }
  if (ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != format) { enableButtonApply(true); return; }
  if (ui.klineedit_pattern->text() != pattern) { enableButtonApply(true); return; }
  enableButtonApply(false);
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
  pattern = ui.klineedit_pattern->text();
  absFilePath = ui.checkBox_abs_file_path->isChecked();
  utf8 = ui.checkBox_utf8->isChecked();
  enableButtonApply(false);
  return true;
}
