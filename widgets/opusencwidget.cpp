/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "opusencwidget.h"

#include <QDebug>

namespace Audex
{

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

    QObject::connect(horizontalSlider_bitrate, &QSlider::valueChanged, this, &opusencWidget::bitrate_changed_by_slider);
    QObject::connect(horizontalSlider_bitrate, &QSlider::valueChanged, this, &opusencWidget::trigger_changed);

    QObject::connect(kintspinbox_bitrate, &QSpinBox::valueChanged, this, &opusencWidget::bitrate_changed_by_spinbox);
    QObject::connect(kintspinbox_bitrate, &QSpinBox::valueChanged, this, &opusencWidget::trigger_changed);

    QObject::connect(checkBox_embedcover, &QCheckBox::toggled, this, &opusencWidget::trigger_changed);

    QObject::connect(qlineedit_suffix, &QLineEdit::textEdited, this, &opusencWidget::trigger_changed);

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

}
