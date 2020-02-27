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

#include "customwidget.h"

#include <QDebug>

customWidget::customWidget(Parameters *parameters, QWidget *parent) : customWidgetUI(parent) {

  Q_UNUSED(parent);

  this->parameters = parameters;
  if (!parameters) {
    qDebug() << "ParameterString is NULL!";
    return;
  }

  qlineedit_pattern->setText(parameters->value(ENCODER_CUSTOM_COMMAND_PATTERN_KEY, ENCODER_CUSTOM_COMMAND_PATTERN));
  qlineedit_suffix->setText(parameters->value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX));

  connect(qlineedit_suffix, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));
  connect(qlineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  connect(kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));

  kpushbutton_pattern->setIcon(QIcon::fromTheme("tools-wizard"));

  changed = false;

}

customWidget::~customWidget() {

}

bool customWidget::save() {

  bool success = true;

  parameters->setValue(ENCODER_CUSTOM_COMMAND_PATTERN_KEY, qlineedit_pattern->text());
  parameters->setValue(ENCODER_CUSTOM_SUFFIX_KEY, qlineedit_suffix->text());

  changed = false;

  return success;

}

void customWidget::pattern_wizard() {

  CommandWizardDialog *dialog = new CommandWizardDialog(qlineedit_pattern->text(), this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }

  qlineedit_pattern->setText(dialog->command);

  delete dialog;

  trigger_changed();

}

void customWidget::trigger_changed() {

  changed = (
    qlineedit_pattern->text() != parameters->value(ENCODER_CUSTOM_COMMAND_PATTERN_KEY, ENCODER_CUSTOM_COMMAND_PATTERN) ||
    qlineedit_suffix->text() != parameters->value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX)
  );

  emit triggerChanged();

}
