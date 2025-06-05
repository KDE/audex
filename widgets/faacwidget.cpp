/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "faacwidget.h"

#include <QDebug>

namespace Audex
{

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
    checkBox_embedcover->setChecked(parameters->value(ENCODER_FAAC_EMBED_COVER_KEY).toBool());
    qlineedit_suffix->setText(parameters->value(ENCODER_FAAC_SUFFIX_KEY, ENCODER_FAAC_SUFFIX).toString());

    QObject::connect(horizontalSlider_quality, &QSlider::valueChanged, this, &faacWidget::quality_changed_by_slider);
    QObject::connect(horizontalSlider_quality, &QSlider::valueChanged, this, &faacWidget::trigger_changed);

    QObject::connect(kintspinbox_quality, &QSpinBox::valueChanged, this, &faacWidget::quality_changed_by_spinbox);
    QObject::connect(kintspinbox_quality, &QSpinBox::valueChanged, this, &faacWidget::trigger_changed);

    QObject::connect(checkBox_embedcover, &QCheckBox::toggled, this, &faacWidget::trigger_changed);

    QObject::connect(qlineedit_suffix, &QLineEdit::textEdited, this, &faacWidget::trigger_changed);

    changed = false;
}

faacWidget::~faacWidget()
{
}

bool faacWidget::save()
{
    bool success = true;

    parameters->setValue(ENCODER_FAAC_QUALITY_KEY, horizontalSlider_quality->value());
    parameters->setValue(ENCODER_FAAC_EMBED_COVER_KEY, checkBox_embedcover->isChecked());
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
               || checkBox_embedcover->isChecked() != parameters->value(ENCODER_FAAC_EMBED_COVER_KEY).toBool()
               || qlineedit_suffix->text() != parameters->value(ENCODER_FAAC_SUFFIX_KEY, ENCODER_FAAC_SUFFIX).toString());

    Q_EMIT triggerChanged();
}

}
