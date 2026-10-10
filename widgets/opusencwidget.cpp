/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "opusencwidget.h"

#include "utils/encoderassistant.h"
#include "utils/parameters.h"

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QLineEdit>
#include <QSpinBox>

opusencWidget::opusencWidget(const Parameters &parameters, QWidget *parent)
    : opusencWidgetUI(parent)
    , p_parameters(parameters)
{
    Q_UNUSED(parent);

    horizontalSlider_bitrate->setValue(p_parameters.value(ENCODER_OPUSENC_BITRATE_KEY, ENCODER_OPUSENC_BITRATE).toInt());
    kintspinbox_bitrate->setValue(p_parameters.value(ENCODER_OPUSENC_BITRATE_KEY, ENCODER_OPUSENC_BITRATE).toInt());
    checkBox_embedcover->setChecked(p_parameters.value(ENCODER_OPUSENC_EMBED_COVER_KEY, ENCODER_OPUSENC_EMBED_COVER).toBool());
    qlineedit_suffix->setText(p_parameters.value(ENCODER_OPUSENC_SUFFIX_KEY, ENCODER_OPUSENC_SUFFIX).toString());

    connect(horizontalSlider_bitrate, &QAbstractSlider::valueChanged, this, &opusencWidget::bitrate_changed_by_slider);
    connect(horizontalSlider_bitrate, &QAbstractSlider::valueChanged, this, &opusencWidget::trigger_changed);

    connect(kintspinbox_bitrate, &QSpinBox::valueChanged, this, &opusencWidget::bitrate_changed_by_spinbox);
    connect(kintspinbox_bitrate, &QSpinBox::valueChanged, this, &opusencWidget::trigger_changed);

    connect(checkBox_embedcover, &QAbstractButton::toggled, this, &opusencWidget::trigger_changed);

    connect(qlineedit_suffix, &QLineEdit::textEdited, this, &opusencWidget::trigger_changed);

    changed = false;
}

opusencWidget::~opusencWidget()
{
}

bool opusencWidget::save()
{
    bool success = true;

    p_parameters.setValue(ENCODER_OPUSENC_BITRATE_KEY, horizontalSlider_bitrate->value());
    p_parameters.setValue(ENCODER_OPUSENC_EMBED_COVER_KEY, checkBox_embedcover->isChecked());
    p_parameters.setValue(ENCODER_OPUSENC_SUFFIX_KEY, qlineedit_suffix->text());

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
    changed = (horizontalSlider_bitrate->value() != p_parameters.value(ENCODER_OPUSENC_BITRATE_KEY, ENCODER_OPUSENC_BITRATE).toInt()
               || checkBox_embedcover->isChecked() != p_parameters.value(ENCODER_OPUSENC_EMBED_COVER_KEY, ENCODER_OPUSENC_EMBED_COVER).toBool()
               || qlineedit_suffix->text() != p_parameters.value(ENCODER_OPUSENC_SUFFIX_KEY, ENCODER_OPUSENC_SUFFIX).toString());

    Q_EMIT triggerChanged();
}
