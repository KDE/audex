/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "faacwidget.h"

#include <QDebug>

faacWidget::faacWidget(Parameters *parameters, QWidget *parent)
    : faacWidgetUI(parent)
{
    Q_UNUSED(parent);

    this->parameters = parameters;
    if (!parameters) {
        qDebug() << "ParameterString is NULL!";
        return;
    }

    horizontalSlider_quality->setValue(parameters->value(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY).toInt());
    kintspinbox_quality->setValue(parameters->value(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY).toInt());
    qlineedit_suffix->setText(parameters->value(ENCODER_FAAC_SUFFIX_KEY, ENCODER_FAAC_SUFFIX).toString());

    connect(horizontalSlider_quality, SIGNAL(valueChanged(int)), this, SLOT(quality_changed_by_slider(int)));
    connect(horizontalSlider_quality, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

    connect(kintspinbox_quality, SIGNAL(valueChanged(int)), this, SLOT(quality_changed_by_spinbox(int)));
    connect(kintspinbox_quality, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

    connect(qlineedit_suffix, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    changed = false;
}

faacWidget::~faacWidget()
{
}

bool faacWidget::save()
{
    bool success = true;

    parameters->setValue(ENCODER_FAAC_QUALITY_KEY, horizontalSlider_quality->value());
    parameters->setValue(ENCODER_FAAC_SUFFIX_KEY, qlineedit_suffix->text());

    changed = false;

    return success;
}

void faacWidget::quality_changed_by_slider(int quality)
{
    kintspinbox_quality->blockSignals(true);
    kintspinbox_quality->setValue(quality);
    kintspinbox_quality->blockSignals(false);
}

void faacWidget::quality_changed_by_spinbox(int quality)
{
    horizontalSlider_quality->blockSignals(true);
    horizontalSlider_quality->setValue(quality);
    horizontalSlider_quality->blockSignals(false);
}

void faacWidget::trigger_changed()
{
    changed = (horizontalSlider_quality->value() != parameters->value(ENCODER_FAAC_QUALITY_KEY, ENCODER_FAAC_QUALITY).toInt()
               || qlineedit_suffix->text() != parameters->value(ENCODER_FAAC_SUFFIX_KEY, ENCODER_FAAC_SUFFIX).toString());

    Q_EMIT triggerChanged();
}
