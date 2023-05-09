/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "flacwidget.h"

#include <QDebug>

flacWidget::flacWidget(Parameters *parameters, QWidget *parent)
    : flacWidgetUI(parent)
{
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
    connect(qlineedit_suffix, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    changed = false;
}

flacWidget::~flacWidget()
{
}

bool flacWidget::save()
{
    bool success = true;

    parameters->setValue(ENCODER_FLAC_COMPRESSION_KEY, horizontalSlider_compression->value());
    parameters->setValue(ENCODER_FLAC_EMBED_COVER_KEY, checkBox_embedcover->isChecked());
    parameters->setValue(ENCODER_FLAC_SUFFIX_KEY, qlineedit_suffix->text());

    changed = false;

    return success;
}

void flacWidget::compression_changed_by_slider(int compression)
{
    kintspinbox_compression->blockSignals(true);
    kintspinbox_compression->setValue(compression);
    kintspinbox_compression->blockSignals(false);
}

void flacWidget::compression_changed_by_spinbox(int compression)
{
    horizontalSlider_compression->blockSignals(true);
    horizontalSlider_compression->setValue(compression);
    horizontalSlider_compression->blockSignals(false);
}

void flacWidget::trigger_changed()
{
    changed = (horizontalSlider_compression->value() != parameters->valueToInt(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION) || checkBox_embedcover->isChecked() != parameters->valueToBool(ENCODER_LAME_EMBED_COVER_KEY) ||
               qlineedit_suffix->text() != parameters->value(ENCODER_FLAC_SUFFIX_KEY, ENCODER_FLAC_SUFFIX));

    Q_EMIT triggerChanged();
}
