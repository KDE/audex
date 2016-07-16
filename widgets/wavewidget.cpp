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

#include "wavewidget.h"

#include <QDebug>

waveWidget::waveWidget(Parameters *parameters, QWidget *parent) : waveWidgetUI(parent) {

  Q_UNUSED(parent);

  this->parameters = parameters;
  if (!parameters) {
    qDebug() << "ParameterString is NULL!";
    return;
  }

  qlineedit_suffix->setText(parameters->value(ENCODER_WAVE_SUFFIX_KEY, ENCODER_WAVE_SUFFIX));

  connect(qlineedit_suffix, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  changed = false;

}

waveWidget::~waveWidget() {

}

bool waveWidget::save() {

  bool success = true;

  parameters->setValue(ENCODER_WAVE_SUFFIX_KEY, qlineedit_suffix->text());

  changed = false;

  return success;

}

void waveWidget::trigger_changed() {

  changed = (
    qlineedit_suffix->text() != parameters->value(ENCODER_WAVE_SUFFIX_KEY, ENCODER_WAVE_SUFFIX)
  );

  emit triggerChanged();

}
