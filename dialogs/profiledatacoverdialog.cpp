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

ProfileDataCoverDialog::ProfileDataCoverDialog(const bool scale, const QSize& size, const QString& format, const QString &pattern, QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  this->scale = scale;
  this->size = size;
  this->format = format;
  this->pattern = pattern;

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  connect(ui.checkBox_scale, SIGNAL(toggled(bool)), this, SLOT(enable_scale(bool)));

  setCaption(i18n("Cover Settings"));

  setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

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

  enableButtonApply(false);
  showButtonSeparator(true);

}

ProfileDataCoverDialog::~ProfileDataCoverDialog() {
}

void ProfileDataCoverDialog::slotButtonClicked(int button) {
  if (button == KDialog::Ok) {
    save();
    accept();
  } else if (button == KDialog::Apply) {
    save();
  } else {
    KDialog::slotButtonClicked(button);
  }
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
  if (ui.checkBox_scale->isChecked() != scale) { enableButtonApply(true); return; }
  if (ui.kintspinbox_x->value() != size.width()) { enableButtonApply(true); return; }
  if (ui.kintspinbox_y->value() != size.height()) { enableButtonApply(true); return; }
  if (ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != format) { enableButtonApply(true); return; }
  if (ui.qlineedit_pattern->text() != pattern) { enableButtonApply(true); return; }
  enableButtonApply(false);
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
  enableButtonApply(false);
  return true;
}
