/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2013 Marco Nelles (audex@maniatek.com)
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

#include "profiledatahashlistdialog.h"

ProfileDataHashlistDialog::ProfileDataHashlistDialog(const QString &pattern, const QString& format, QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  this->format = format;
  this->pattern = pattern;

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  setCaption(i18n("Playlist Settings"));

  setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

  connect(ui.kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));
  ui.kpushbutton_pattern->setIcon(KIcon("tools-wizard"));

  ui.kcombobox_format->addItem(i18n("SFV (Simple File Verification)"), "SFV");
  ui.kcombobox_format->addItem(i18n("MD5 (Message-Digest algorithm 5)"), "MD5");
  {
    int i = ui.kcombobox_format->findData(format);
    ui.kcombobox_format->setCurrentIndex(i);
  }
  connect(ui.kcombobox_format, SIGNAL(currentIndexChanged(int)), this, SLOT(trigger_changed()));

  ui.klineedit_pattern->setText(pattern);
  connect(ui.klineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  enableButtonApply(FALSE);
  showButtonSeparator(true);

}

ProfileDataHashlistDialog::~ProfileDataHashlistDialog() {
}

void ProfileDataHashlistDialog::slotButtonClicked(int button) {
  if (button == KDialog::Ok) {
    save();
    accept();
  } else if (button == KDialog::Apply) {
    save();
  } else {
    KDialog::slotButtonClicked(button);
  }
}

void ProfileDataHashlistDialog::pattern_wizard() {

  QString suffix = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString().toLower();

  SimplePatternWizardDialog *dialog = new SimplePatternWizardDialog(ui.klineedit_pattern->text(), suffix, this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }

  ui.klineedit_pattern->setText(dialog->pattern);

  delete dialog;

  trigger_changed();

}

void ProfileDataHashlistDialog::trigger_changed() {
  if (ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != format) { enableButtonApply(TRUE); return; }
  if (ui.klineedit_pattern->text() != pattern) { enableButtonApply(TRUE); return; }
  enableButtonApply(FALSE);
}

bool ProfileDataHashlistDialog::save() {
  format = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString();
  pattern = ui.klineedit_pattern->text();
  enableButtonApply(FALSE);
  return TRUE;
}
