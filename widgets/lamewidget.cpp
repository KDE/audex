/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "lamewidget.h"

#include <QDebug>

lameWidget::lameWidget(Parameters *parameters, QWidget *parent)
    : lameWidgetUI(parent)
{
    Q_UNUSED(parent);

    this->parameters = parameters;
    if (!parameters) {
        qDebug() << "ParameterString is NULL!";
        return;
    }

    bitrates << 80 << 96 << 112 << 128 << 160 << 192 << 224 << 256 << 320;

    real_bitrate = parameters->value(ENCODER_LAME_BITRATE_KEY, ENCODER_LAME_BITRATE).toInt();
    p_cbr_flag = parameters->value(ENCODER_LAME_CBR_KEY).toBool();
    enable_CBR(p_cbr_flag);
    preset = parameters->value(ENCODER_LAME_PRESET_KEY, ENCODER_LAME_PRESET).toInt();
    switch (preset) {
    case ENCODER_LAME_PRESET_MEDIUM:
        radioButton_medium->setChecked(true);
        enable_custom(false);
        break;
    case ENCODER_LAME_PRESET_STANDARD:
        radioButton_standard->setChecked(true);
        enable_custom(false);
        break;
    case ENCODER_LAME_PRESET_EXTREME:
        radioButton_extreme->setChecked(true);
        enable_custom(false);
        break;
    case ENCODER_LAME_PRESET_INSANE:
        radioButton_insane->setChecked(true);
        enable_custom(false);
        break;
    case ENCODER_LAME_PRESET_CUSTOM:
        radioButton_custom->setChecked(true);
        enable_custom(true);
        break;
    }
    checkBox_cbr->setChecked(p_cbr_flag);
    checkBox_embedcover->setChecked(parameters->value(ENCODER_LAME_EMBED_COVER_KEY).toBool());
    qlineedit_suffix->setText(parameters->value(ENCODER_LAME_SUFFIX_KEY, ENCODER_LAME_SUFFIX).toString());

    connect(radioButton_medium, SIGNAL(toggled(bool)), this, SLOT(enable_medium(bool)));
    connect(radioButton_medium, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));
    connect(radioButton_standard, SIGNAL(toggled(bool)), this, SLOT(enable_standard(bool)));
    connect(radioButton_standard, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));
    connect(radioButton_extreme, SIGNAL(toggled(bool)), this, SLOT(enable_extreme(bool)));
    connect(radioButton_extreme, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));
    connect(radioButton_insane, SIGNAL(toggled(bool)), this, SLOT(enable_insane(bool)));
    connect(radioButton_insane, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));
    connect(radioButton_custom, SIGNAL(toggled(bool)), this, SLOT(enable_custom(bool)));
    connect(radioButton_custom, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

    connect(checkBox_cbr, SIGNAL(toggled(bool)), this, SLOT(enable_CBR(bool)));
    connect(checkBox_cbr, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

    connect(horizontalSlider_bitrate, SIGNAL(valueChanged(int)), this, SLOT(bitrate_changed_by_slider(int)));
    connect(horizontalSlider_bitrate, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

    connect(kintspinbox_bitrate, SIGNAL(valueChanged(int)), this, SLOT(bitrate_changed_by_spinbox(int)));
    connect(kintspinbox_bitrate, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

    connect(checkBox_embedcover, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

    connect(qlineedit_suffix, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    changed = false;
}

lameWidget::~lameWidget()
{
}

bool lameWidget::save()
{
    bool success = true;

    parameters->setValue(ENCODER_LAME_PRESET_KEY, preset);
    parameters->setValue(ENCODER_LAME_BITRATE_KEY, real_bitrate);
    parameters->setValue(ENCODER_LAME_CBR_KEY, checkBox_cbr->isChecked());
    parameters->setValue(ENCODER_LAME_EMBED_COVER_KEY, checkBox_embedcover->isChecked());
    parameters->setValue(ENCODER_LAME_SUFFIX_KEY, qlineedit_suffix->text());

    changed = false;

    return success;
}

void lameWidget::enable_medium(bool enable)
{
    if (enable)
        preset = ENCODER_LAME_PRESET_MEDIUM;
}

void lameWidget::enable_standard(bool enable)
{
    if (enable)
        preset = ENCODER_LAME_PRESET_STANDARD;
}

void lameWidget::enable_extreme(bool enable)
{
    if (enable)
        preset = ENCODER_LAME_PRESET_EXTREME;
}

void lameWidget::enable_insane(bool enable)
{
    if (enable)
        preset = ENCODER_LAME_PRESET_INSANE;
}

void lameWidget::enable_custom(bool enable)
{
    checkBox_cbr->setEnabled(enable);
    label_targetbitrate->setEnabled(enable);
    horizontalSlider_bitrate->setEnabled(enable);
    kintspinbox_bitrate->setEnabled(enable);
    label_lowest->setEnabled(enable);
    label_highest->setEnabled(enable);
    if (enable)
        preset = ENCODER_LAME_PRESET_CUSTOM;
}

template<class T>
T square(const T &x)
{
    return x * x;
}

void lameWidget::enable_CBR(bool enable)
{
    if (enable) {
        // find the nearest neighbor
        int d = 0, nd = 0, nb = -1, i;
        for (i = 0; i < bitrates.count(); ++i) {
            nd = square(real_bitrate - bitrates[i]);
            if ((i > 0) && (nd > d)) {
                nb = bitrates[i - 1];
                break;
            }
            d = nd;
        }
        if (nb == -1)
            nb = bitrates.last();

        horizontalSlider_bitrate->blockSignals(true);
        horizontalSlider_bitrate->setMinimum(0);
        horizontalSlider_bitrate->setMaximum(8);
        horizontalSlider_bitrate->setSingleStep(1);
        horizontalSlider_bitrate->setPageStep(1);
        horizontalSlider_bitrate->setValue(i - 1);
        horizontalSlider_bitrate->blockSignals(false);

        kintspinbox_bitrate->blockSignals(true);
        kintspinbox_bitrate->setMinimum(80);
        kintspinbox_bitrate->setMaximum(320);
        kintspinbox_bitrate->setSingleStep(16);
        kintspinbox_bitrate->setValue(nb);
        kintspinbox_bitrate->setReadOnly(true);
        kintspinbox_bitrate->blockSignals(false);

        real_bitrate = nb;

        p_cbr_flag = true;

    } else {
        horizontalSlider_bitrate->blockSignals(true);
        horizontalSlider_bitrate->setMinimum(80);
        horizontalSlider_bitrate->setMaximum(320);
        horizontalSlider_bitrate->setSingleStep(1);
        horizontalSlider_bitrate->setPageStep(10);
        horizontalSlider_bitrate->setValue(real_bitrate);
        horizontalSlider_bitrate->blockSignals(false);

        kintspinbox_bitrate->blockSignals(true);
        kintspinbox_bitrate->setMinimum(80);
        kintspinbox_bitrate->setMaximum(320);
        kintspinbox_bitrate->setSingleStep(1);
        kintspinbox_bitrate->setValue(real_bitrate);
        kintspinbox_bitrate->setReadOnly(false);
        kintspinbox_bitrate->blockSignals(false);

        p_cbr_flag = false;
    }
}

void lameWidget::bitrate_changed_by_slider(int bitrate)
{
    if (p_cbr_flag) {
        if (bitrate < 0)
            bitrate = 0;
        if (bitrate >= bitrates.count())
            bitrate = bitrates.count() - 1;

        kintspinbox_bitrate->blockSignals(true);
        kintspinbox_bitrate->setValue(bitrates[bitrate]);
        kintspinbox_bitrate->blockSignals(false);

        real_bitrate = bitrates[bitrate];

    } else {
        kintspinbox_bitrate->blockSignals(true);
        kintspinbox_bitrate->setValue(bitrate);
        kintspinbox_bitrate->blockSignals(false);

        real_bitrate = bitrate;
    }
}

void lameWidget::bitrate_changed_by_spinbox(int bitrate)
{
    if (!p_cbr_flag) {
        horizontalSlider_bitrate->blockSignals(true);
        horizontalSlider_bitrate->setValue(bitrate);
        horizontalSlider_bitrate->blockSignals(false);

        real_bitrate = bitrate;
    }
}

void lameWidget::trigger_changed()
{
    changed = (preset != parameters->value(ENCODER_LAME_PRESET_KEY, ENCODER_LAME_PRESET).toInt()
               || real_bitrate != parameters->value(ENCODER_LAME_BITRATE_KEY, ENCODER_LAME_BITRATE).toInt()
               || checkBox_cbr->isChecked() != parameters->value(ENCODER_LAME_CBR_KEY).toBool()
               || checkBox_embedcover->isChecked() != parameters->value(ENCODER_LAME_EMBED_COVER_KEY).toBool()
               || qlineedit_suffix->text() != parameters->value(ENCODER_LAME_SUFFIX_KEY, ENCODER_LAME_SUFFIX).toString());

    Q_EMIT triggerChanged();
}
