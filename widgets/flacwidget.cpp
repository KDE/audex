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

#include "flacwidget.h"

flacWidget::flacWidget(Parameters *parameters, QWidget *parent) : flacWidgetUI(parent) {

  Q_UNUSED(parent);

  this->parameters = parameters;
  if (!parameters) {
    kDebug() << "ParameterString is NULL!";
    return;
  }

  horizontalSlider_compression->setValue(parameters->valueToInt(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION));
  kintspinbox_compression->setValue(parameters->valueToInt(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION));
  klineedit_suffix->setText(parameters->value(ENCODER_FLAC_SUFFIX_KEY, ENCODER_FLAC_SUFFIX));

  connect(horizontalSlider_compression, SIGNAL(valueChanged(int)), this, SLOT(compression_changed_by_slider(int)));
  connect(horizontalSlider_compression, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

  connect(kintspinbox_compression, SIGNAL(valueChanged(int)), this, SLOT(compression_changed_by_spinbox(int)));
  connect(kintspinbox_compression, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

  connect(klineedit_suffix, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  changed = FALSE;

}

flacWidget::~flacWidget() {

}

bool flacWidget::save() {

  bool success = TRUE;

  parameters->setValue(ENCODER_FLAC_COMPRESSION_KEY, horizontalSlider_compression->value());
  parameters->setValue(ENCODER_FLAC_SUFFIX_KEY, klineedit_suffix->text());

  changed = FALSE;

  return success;

}

void flacWidget::compression_changed_by_slider(int compression) {

  kintspinbox_compression->blockSignals(TRUE);
  kintspinbox_compression->setValue(compression);
  kintspinbox_compression->blockSignals(FALSE);

}

void flacWidget::compression_changed_by_spinbox(int compression) {

  horizontalSlider_compression->blockSignals(TRUE);
  horizontalSlider_compression->setValue(compression);
  horizontalSlider_compression->blockSignals(FALSE);

}

void flacWidget::trigger_changed() {

  changed = (
    horizontalSlider_compression->value() != parameters->valueToInt(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION) ||
    klineedit_suffix->text() != parameters->value(ENCODER_FLAC_SUFFIX_KEY, ENCODER_FLAC_SUFFIX)
  );

  emit triggerChanged();

}
