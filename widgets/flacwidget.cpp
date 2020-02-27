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

#include "flacwidget.h"

#include <QDebug>

flacWidget::flacWidget(Parameters *parameters, QWidget *parent) : flacWidgetUI(parent) {

  Q_UNUSED(parent);

  this->parameters = parameters;
  if (!parameters) {
    qDebug() << "ParameterString is NULL!";
    return;
  }

  horizontalSlider_compression->setValue(parameters->valueToInt(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION));
  kintspinbox_compression->setValue(parameters->valueToInt(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION));

  checkBox_embedcover->setChecked(parameters->valueToBool(ENCODER_FLAC_EMBED_COVER_KEY));
  qlineedit_suffix->setText(parameters->value(ENCODER_FLAC_SUFFIX_KEY, ENCODER_FLAC_SUFFIX));

  connect(horizontalSlider_compression, SIGNAL(valueChanged(int)), this, SLOT(compression_changed_by_slider(int)));
  connect(horizontalSlider_compression, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

  connect(kintspinbox_compression, SIGNAL(valueChanged(int)), this, SLOT(compression_changed_by_spinbox(int)));
  connect(kintspinbox_compression, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

  connect(checkBox_embedcover, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));
  connect(qlineedit_suffix, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  changed = false;

}

flacWidget::~flacWidget() {

}

bool flacWidget::save() {

  bool success = true;

  parameters->setValue(ENCODER_FLAC_COMPRESSION_KEY, horizontalSlider_compression->value());
  parameters->setValue(ENCODER_FLAC_EMBED_COVER_KEY, checkBox_embedcover->isChecked());
  parameters->setValue(ENCODER_FLAC_SUFFIX_KEY, qlineedit_suffix->text());

  changed = false;

  return success;

}

void flacWidget::compression_changed_by_slider(int compression) {

  kintspinbox_compression->blockSignals(true);
  kintspinbox_compression->setValue(compression);
  kintspinbox_compression->blockSignals(false);

}

void flacWidget::compression_changed_by_spinbox(int compression) {

  horizontalSlider_compression->blockSignals(true);
  horizontalSlider_compression->setValue(compression);
  horizontalSlider_compression->blockSignals(false);

}

void flacWidget::trigger_changed() {

  changed = (
    horizontalSlider_compression->value() != parameters->valueToInt(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION) ||
    checkBox_embedcover->isChecked() != parameters->valueToBool(ENCODER_LAME_EMBED_COVER_KEY) ||
    qlineedit_suffix->text() != parameters->value(ENCODER_FLAC_SUFFIX_KEY, ENCODER_FLAC_SUFFIX)
  );

  emit triggerChanged();

}
