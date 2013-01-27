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

#include "customwidget.h"

customWidget::customWidget(Parameters *parameters, QWidget *parent) : customWidgetUI(parent) {

  Q_UNUSED(parent);

  this->parameters = parameters;
  if (!parameters) {
    kDebug() << "ParameterString is NULL!";
    return;
  }

  klineedit_pattern->setText(parameters->value(ENCODER_CUSTOM_COMMAND_PATTERN_KEY, ENCODER_CUSTOM_COMMAND_PATTERN));
  klineedit_suffix->setText(parameters->value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX));

  connect(klineedit_suffix, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));
  connect(klineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  connect(kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));

  kpushbutton_pattern->setIcon(KIcon("tools-wizard"));

  changed = FALSE;

}

customWidget::~customWidget() {

}

bool customWidget::save() {

  bool success = TRUE;

  parameters->setValue(ENCODER_CUSTOM_COMMAND_PATTERN_KEY, klineedit_pattern->text());
  parameters->setValue(ENCODER_CUSTOM_SUFFIX_KEY, klineedit_suffix->text());

  changed = FALSE;

  return success;

}

void customWidget::pattern_wizard() {

  CommandWizardDialog *dialog = new CommandWizardDialog(klineedit_pattern->text(), this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }

  klineedit_pattern->setText(dialog->command);

  delete dialog;

  trigger_changed();

}

void customWidget::trigger_changed() {

  changed = (
    klineedit_pattern->text() != parameters->value(ENCODER_CUSTOM_COMMAND_PATTERN_KEY, ENCODER_CUSTOM_COMMAND_PATTERN) ||
    klineedit_suffix->text() != parameters->value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX)
  );

  emit triggerChanged();

}
