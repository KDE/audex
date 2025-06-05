/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "flacwidget.h"

#include <QDebug>

namespace Audex
{

flacWidget::flacWidget(Parameters *parameters, QWidget *parent)
    : flacWidgetUI(parent)
{
    Q_UNUSED(parent);

    this->parameters = parameters;
    if (!parameters) {
        qDebug() << "ParameterString is NULL!";
        return;
    }

    horizontalSlider_compression->setValue(parameters->value(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION).toInt());
    kintspinbox_compression->setValue(parameters->value(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION).toInt());

    checkBox_embedcover->setChecked(parameters->value(ENCODER_FLAC_EMBED_COVER_KEY).toBool());
    qlineedit_suffix->setText(parameters->value(ENCODER_FLAC_SUFFIX_KEY, ENCODER_FLAC_SUFFIX).toString());

    QObject::connect(horizontalSlider_compression, &QSlider::valueChanged, this, &flacWidget::compression_changed_by_slider);
    QObject::connect(horizontalSlider_compression, &QSlider::valueChanged, this, &flacWidget::trigger_changed);

    QObject::connect(kintspinbox_compression, &QSpinBox::valueChanged, this, &flacWidget::compression_changed_by_spinbox);
    QObject::connect(kintspinbox_compression, &QSpinBox::valueChanged, this, &flacWidget::trigger_changed);

    QObject::connect(checkBox_embedcover, &QCheckBox::toggled, this, &flacWidget::trigger_changed);
    QObject::connect(qlineedit_suffix, &QLineEdit::textEdited, this, &flacWidget::trigger_changed);

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
    changed = (horizontalSlider_compression->value() != parameters->value(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION).toInt()
               || checkBox_embedcover->isChecked() != parameters->value(ENCODER_LAME_EMBED_COVER_KEY).toBool()
               || qlineedit_suffix->text() != parameters->value(ENCODER_FLAC_SUFFIX_KEY, ENCODER_FLAC_SUFFIX).toString());

    Q_EMIT triggerChanged();
}

}
