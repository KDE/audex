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

#include "profiledatacoverdialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataCoverDialog::ProfileDataCoverDialog(const bool scale, const QSize& size, const QString& format, const QString &pattern, QWidget *parent) : QDialog(parent) {

  Q_UNUSED(parent);

  this->scale = scale;
  this->size = size;
  this->format = format;
  this->pattern = pattern;

  setWindowTitle(i18n("Cover Settings"));

  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);


  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply);
  QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
  applyButton = buttonBox -> button(QDialogButtonBox::Apply);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataCoverDialog::slotAccepted);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataCoverDialog::reject);
  connect(applyButton, &QPushButton::clicked, this, &ProfileDataCoverDialog::slotApplied);

  QWidget *widget = new QWidget(this);
  mainLayout->addWidget(widget);
  mainLayout->addWidget(buttonBox);
  ui.setupUi(widget);

  connect(ui.checkBox_scale, &QCheckBox::toggled, this, &ProfileDataCoverDialog::enable_scale);
  connect(ui.kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));
  ui.kpushbutton_pattern->setIcon(QIcon::fromTheme("tools-wizard"));

  ui.checkBox_scale->setChecked(scale);
  enable_scale(ui.checkBox_scale->isChecked());
  connect(ui.checkBox_scale, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

  ui.kintspinbox_x->setValue(size.width());
  connect(ui.kintspinbox_x, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

  ui.kintspinbox_y->setValue(size.height());
  connect(ui.kintspinbox_y, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

  ui.kcombobox_format->addItem(i18n("JPEG (Joint Photographic Experts Group)"), "JPEG");
  ui.kcombobox_format->addItem(i18n("PNG (Portable Network Graphics)"), "PNG");
  ui.kcombobox_format->addItem(i18n("BMP (Windows Bitmap)"), "BMP");
  {
    int i = ui.kcombobox_format->findData(format);
    ui.kcombobox_format->setCurrentIndex(i);
  }
  connect(ui.kcombobox_format, SIGNAL(currentIndexChanged(int)), this, SLOT(trigger_changed()));

  ui.qlineedit_pattern->setText(pattern);
  connect(ui.qlineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  applyButton->setEnabled(false);

}

ProfileDataCoverDialog::~ProfileDataCoverDialog() {
}

void ProfileDataCoverDialog::slotAccepted() {
  save();
  accept();
}

void ProfileDataCoverDialog::slotApplied() {
  save();
}

void ProfileDataCoverDialog::pattern_wizard() {

  QString suffix = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString().toLower();

  SimplePatternWizardDialog *dialog = new SimplePatternWizardDialog(ui.qlineedit_pattern->text(), suffix, this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }

  ui.qlineedit_pattern->setText(dialog->pattern);

  delete dialog;

  trigger_changed();

}

void ProfileDataCoverDialog::trigger_changed() {
  if (ui.checkBox_scale->isChecked() != scale) { applyButton->setEnabled(true); return; }
  if (ui.kintspinbox_x->value() != size.width()) { applyButton->setEnabled(true); return; }
  if (ui.kintspinbox_y->value() != size.height()) { applyButton->setEnabled(true); return; }
  if (ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != format) { applyButton->setEnabled(true); return; }
  if (ui.qlineedit_pattern->text() != pattern) { applyButton->setEnabled(true); return; }
  applyButton->setEnabled(false);
}

void ProfileDataCoverDialog::enable_scale(bool enabled) {
  ui.label_x->setEnabled(enabled);
  ui.kintspinbox_x->setEnabled(enabled);
  ui.kintspinbox_y->setEnabled(enabled);
}

bool ProfileDataCoverDialog::save() {
  scale = ui.checkBox_scale->isChecked();
  size = QSize(ui.kintspinbox_x->value(), ui.kintspinbox_y->value());
  format = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString();
  pattern = ui.qlineedit_pattern->text();
  applyButton->setEnabled(false);
  return true;
}
