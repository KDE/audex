/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "opusencwidget.h"

#include <QDebug>

opusencWidget::opusencWidget(Parameters *parameters, QWidget *parent)
    : opusencWidgetUI(parent)
{
    Q_UNUSED(parent);

    this->parameters = parameters;
    if (!parameters) {
        qDebug() << "ParameterString is NULL!";
        return;
    }

    horizontalSlider_bitrate->setValue(parameters->value(ENCODER_OPUSENC_BITRATE_KEY, ENCODER_OPUSENC_BITRATE).toInt());
    kintspinbox_bitrate->setValue(parameters->value(ENCODER_OPUSENC_BITRATE_KEY, ENCODER_OPUSENC_BITRATE).toInt());
    checkBox_embedcover->setChecked(parameters->value(ENCODER_OPUSENC_EMBED_COVER_KEY).toBool());
    qlineedit_suffix->setText(parameters->value(ENCODER_OPUSENC_SUFFIX_KEY, ENCODER_OPUSENC_SUFFIX).toString());

    connect(horizontalSlider_bitrate, SIGNAL(valueChanged(int)), this, SLOT(bitrate_changed_by_slider(int)));
    connect(horizontalSlider_bitrate, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

    connect(kintspinbox_bitrate, SIGNAL(valueChanged(int)), this, SLOT(bitrate_changed_by_spinbox(int)));
    connect(kintspinbox_bitrate, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

    connect(checkBox_embedcover, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

    connect(qlineedit_suffix, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    changed = false;
}

opusencWidget::~opusencWidget()
{
}

bool opusencWidget::save()
{
    bool success = true;

    parameters->setValue(ENCODER_OPUSENC_BITRATE_KEY, horizontalSlider_bitrate->value());
    parameters->setValue(ENCODER_OPUSENC_EMBED_COVER_KEY, checkBox_embedcover->isChecked());
    parameters->setValue(ENCODER_OPUSENC_SUFFIX_KEY, qlineedit_suffix->text());

    changed = false;

    return success;
}

void opusencWidget::bitrate_changed_by_slider(int bitrate)
{
    kintspinbox_bitrate->blockSignals(true);
    kintspinbox_bitrate->setValue(bitrate);
    kintspinbox_bitrate->blockSignals(false);
}

void opusencWidget::bitrate_changed_by_spinbox(int bitrate)
{
    horizontalSlider_bitrate->blockSignals(true);
    horizontalSlider_bitrate->setValue(bitrate);
    horizontalSlider_bitrate->blockSignals(false);
}

void opusencWidget::trigger_changed()
{
    changed = (horizontalSlider_bitrate->value() != parameters->value(ENCODER_OPUSENC_BITRATE_KEY, ENCODER_OPUSENC_BITRATE).toInt()
               || checkBox_embedcover->isChecked() != parameters->value(ENCODER_OPUSENC_EMBED_COVER_KEY).toBool()
               || qlineedit_suffix->text() != parameters->value(ENCODER_OPUSENC_SUFFIX_KEY, ENCODER_OPUSENC_SUFFIX).toString());

    Q_EMIT triggerChanged();
}
