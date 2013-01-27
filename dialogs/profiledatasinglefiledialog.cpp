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

#include "profiledatasinglefiledialog.h"

ProfileDataSingleFileDialog::ProfileDataSingleFileDialog(const QString &pattern, QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  this->pattern = pattern;

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  setCaption(i18n("Single File Settings"));

  setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

  connect(ui.kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));
  ui.kpushbutton_pattern->setIcon(KIcon("tools-wizard"));

  ui.klineedit_pattern->setText(pattern);
  connect(ui.klineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  enableButtonApply(FALSE);
  showButtonSeparator(true);

}

ProfileDataSingleFileDialog::~ProfileDataSingleFileDialog() {
}

void ProfileDataSingleFileDialog::slotButtonClicked(int button) {
  if (button == KDialog::Ok) {
    save();
    accept();
  } else if (button == KDialog::Apply) {
    save();
  } else {
    KDialog::slotButtonClicked(button);
  }
}

void ProfileDataSingleFileDialog::pattern_wizard() {

  SimplePatternWizardDialog *dialog = new SimplePatternWizardDialog(ui.klineedit_pattern->text(), "wav", this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }

  ui.klineedit_pattern->setText(dialog->pattern);

  delete dialog;

  trigger_changed();

}

void ProfileDataSingleFileDialog::trigger_changed() {
  if (ui.klineedit_pattern->text() != pattern) { enableButtonApply(TRUE); return; }
  enableButtonApply(FALSE);
}

bool ProfileDataSingleFileDialog::save() {
  pattern = ui.klineedit_pattern->text();
  enableButtonApply(FALSE);
  return TRUE;
}
