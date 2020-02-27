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

#include "profiledatahashlistdialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataHashlistDialog::ProfileDataHashlistDialog(const QString &pattern, const QString& format, QWidget *parent) : QDialog(parent) {

  Q_UNUSED(parent);

  this->format = format;
  this->pattern = pattern;

  setWindowTitle(i18n("Playlist Settings"));

  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply);
  QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
  applyButton = buttonBox->button(QDialogButtonBox::Apply);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataHashlistDialog::slotAccepted);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataHashlistDialog::reject);
  connect(applyButton, &QPushButton::clicked, this, &ProfileDataHashlistDialog::slotApplied);

  QWidget *widget = new QWidget(this);
  mainLayout->addWidget(widget);
  mainLayout->addWidget(buttonBox);
  ui.setupUi(widget);

  connect(ui.kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));
  ui.kpushbutton_pattern->setIcon(QIcon::fromTheme("tools-wizard"));

  ui.kcombobox_format->addItem(i18n("SFV (Simple File Verification)"), "SFV");
  ui.kcombobox_format->addItem(i18n("MD5 (Message-Digest algorithm 5)"), "MD5");
  {
    int i = ui.kcombobox_format->findData(format);
    ui.kcombobox_format->setCurrentIndex(i);
  }
  connect(ui.kcombobox_format, SIGNAL(currentIndexChanged(int)), this, SLOT(trigger_changed()));

  ui.qlineedit_pattern->setText(pattern);
  connect(ui.qlineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  applyButton->setEnabled(false);

}

ProfileDataHashlistDialog::~ProfileDataHashlistDialog() {
}

void ProfileDataHashlistDialog::slotAccepted() {
  save();
  accept();
}

void ProfileDataHashlistDialog::slotApplied() {
  save();
}

void ProfileDataHashlistDialog::pattern_wizard() {

  QString suffix = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString().toLower();

  SimplePatternWizardDialog *dialog = new SimplePatternWizardDialog(ui.qlineedit_pattern->text(), suffix, this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }

  ui.qlineedit_pattern->setText(dialog->pattern);

  delete dialog;

  trigger_changed();

}

void ProfileDataHashlistDialog::trigger_changed() {
  if (ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != format) { applyButton->setEnabled(true); return; }
  if (ui.qlineedit_pattern->text() != pattern) { applyButton->setEnabled(true); return; }
  applyButton->setEnabled(false);
}

bool ProfileDataHashlistDialog::save() {
  format = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString();
  pattern = ui.qlineedit_pattern->text();
  applyButton->setEnabled(false);
  return true;
}
