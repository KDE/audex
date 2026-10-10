/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "flacwidget.h"

#include "utils/encoderassistant.h"
#include "utils/parameters.h"

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QLineEdit>
#include <QSpinBox>

flacWidget::flacWidget(const Parameters &parameters, QWidget *parent)
    : flacWidgetUI(parent)
    , p_parameters(parameters)
{
    Q_UNUSED(parent);

    horizontalSlider_compression->setValue(p_parameters.value(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION).toInt());
    kintspinbox_compression->setValue(p_parameters.value(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION).toInt());

    checkBox_embedcover->setChecked(p_parameters.value(ENCODER_FLAC_EMBED_COVER_KEY, ENCODER_FLAC_EMBED_COVER).toBool());
    qlineedit_suffix->setText(p_parameters.value(ENCODER_FLAC_SUFFIX_KEY, ENCODER_FLAC_SUFFIX).toString());

    connect(horizontalSlider_compression, &QAbstractSlider::valueChanged, this, &flacWidget::compression_changed_by_slider);
    connect(horizontalSlider_compression, &QAbstractSlider::valueChanged, this, &flacWidget::trigger_changed);

    connect(kintspinbox_compression, &QSpinBox::valueChanged, this, &flacWidget::compression_changed_by_spinbox);
    connect(kintspinbox_compression, &QSpinBox::valueChanged, this, &flacWidget::trigger_changed);

    connect(checkBox_embedcover, &QAbstractButton::toggled, this, &flacWidget::trigger_changed);
    connect(qlineedit_suffix, &QLineEdit::textEdited, this, &flacWidget::trigger_changed);

    changed = false;
}

flacWidget::~flacWidget()
{
}

bool flacWidget::save()
{
    bool success = true;

    p_parameters.setValue(ENCODER_FLAC_COMPRESSION_KEY, horizontalSlider_compression->value());
    p_parameters.setValue(ENCODER_FLAC_EMBED_COVER_KEY, checkBox_embedcover->isChecked());
    p_parameters.setValue(ENCODER_FLAC_SUFFIX_KEY, qlineedit_suffix->text());

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
    changed = (horizontalSlider_compression->value() != p_parameters.value(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION).toInt()
               || checkBox_embedcover->isChecked() != p_parameters.value(ENCODER_FLAC_EMBED_COVER_KEY, ENCODER_FLAC_EMBED_COVER).toBool()
               || qlineedit_suffix->text() != p_parameters.value(ENCODER_FLAC_SUFFIX_KEY, ENCODER_FLAC_SUFFIX).toString());

    Q_EMIT triggerChanged();
}
