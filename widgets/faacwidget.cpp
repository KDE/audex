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

#include "faacwidget.h"

#include <QDebug>

faacWidget::faacWidget(Parameters *parameters, QWidget *parent) : faacWidgetUI(parent) {

  Q_UNUSED(parent);

  this->parameters = parameters;
  if (!parameters) {
    qDebug() << "ParameterString is NULL!";
    return;
  }

  horizontalSlider_quality->setValue(parameters->valueToInt(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY));
  kintspinbox_quality->setValue(parameters->valueToInt(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY));
  qlineedit_suffix->setText(parameters->value(ENCODER_FAAC_SUFFIX_KEY, ENCODER_FAAC_SUFFIX));

  connect(horizontalSlider_quality, SIGNAL(valueChanged(int)), this, SLOT(quality_changed_by_slider(int)));
  connect(horizontalSlider_quality, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

  connect(kintspinbox_quality, SIGNAL(valueChanged(int)), this, SLOT(quality_changed_by_spinbox(int)));
  connect(kintspinbox_quality, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

  connect(qlineedit_suffix, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  changed = false;

}

faacWidget::~faacWidget() {

}

bool faacWidget::save() {

  bool success = true;

  parameters->setValue(ENCODER_FAAC_QUALITY_KEY, horizontalSlider_quality->value());
  parameters->setValue(ENCODER_FAAC_SUFFIX_KEY, qlineedit_suffix->text());

  changed = false;

  return success;

}

void faacWidget::quality_changed_by_slider(int quality) {

  kintspinbox_quality->blockSignals(true);
  kintspinbox_quality->setValue(quality);
  kintspinbox_quality->blockSignals(false);

}

void faacWidget::quality_changed_by_spinbox(int quality) {

  horizontalSlider_quality->blockSignals(true);
  horizontalSlider_quality->setValue(quality);
  horizontalSlider_quality->blockSignals(false);

}

void faacWidget::trigger_changed() {

  changed = (
    horizontalSlider_quality->value() != parameters->valueToInt(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY) ||
    qlineedit_suffix->text() != parameters->value(ENCODER_FAAC_SUFFIX_KEY, ENCODER_FAAC_SUFFIX)
  );

  emit triggerChanged();

}
