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

#include "profiledatacuesheetdialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataCueSheetDialog::ProfileDataCueSheetDialog(const QString &pattern, QWidget *parent) : QDialog(parent) {

  Q_UNUSED(parent);

  this->pattern = pattern;

  setWindowTitle(i18n("Cue Sheet Settings"));

  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply);
  QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
  applyButton = buttonBox->button(QDialogButtonBox::Apply);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataCueSheetDialog::slotAccepted);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataCueSheetDialog::reject);
  connect(applyButton, &QPushButton::clicked, this, &ProfileDataCueSheetDialog::slotApplied);

  QWidget *widget = new QWidget(this);
  mainLayout->addWidget(widget);
  mainLayout->addWidget(buttonBox);
  ui.setupUi(widget);

  connect(ui.kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));
  ui.kpushbutton_pattern->setIcon(QIcon::fromTheme("tools-wizard"));

  ui.qlineedit_pattern->setText(pattern);
  connect(ui.qlineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  applyButton->setEnabled(false);

}

ProfileDataCueSheetDialog::~ProfileDataCueSheetDialog() {
}

void ProfileDataCueSheetDialog::slotAccepted() {
  save();
  accept();
}

void ProfileDataCueSheetDialog::slotApplied() {
  save();
}

void ProfileDataCueSheetDialog::pattern_wizard() {

  SimplePatternWizardDialog *dialog = new SimplePatternWizardDialog(ui.qlineedit_pattern->text(), "cue", this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }

  ui.qlineedit_pattern->setText(dialog->pattern);

  delete dialog;

  trigger_changed();

}

void ProfileDataCueSheetDialog::trigger_changed() {
  if (ui.qlineedit_pattern->text() != pattern) { applyButton->setEnabled(true); return; }
  applyButton->setEnabled(false);
}

bool ProfileDataCueSheetDialog::save() {
  pattern = ui.qlineedit_pattern->text();
  applyButton->setEnabled(false);
  return true;
}
