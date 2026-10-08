/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wavewidget.h"

#include "utils/encoderassistant.h"
#include "utils/parameters.h"

#include <QLineEdit>

waveWidget::waveWidget(const Parameters &parameters, QWidget *parent)
    : waveWidgetUI(parent)
    , p_parameters(parameters)
{
    Q_UNUSED(parent);

    qlineedit_suffix->setText(p_parameters.value(ENCODER_WAVE_SUFFIX_KEY, ENCODER_WAVE_SUFFIX).toString());

    connect(qlineedit_suffix, &QLineEdit::textEdited, this, &waveWidget::trigger_changed);

    changed = false;
}

waveWidget::~waveWidget()
{
}

bool waveWidget::save()
{
    bool success = true;

    p_parameters.setValue(ENCODER_WAVE_SUFFIX_KEY, qlineedit_suffix->text());

    changed = false;

    return success;
}

void waveWidget::trigger_changed()
{
    changed = (qlineedit_suffix->text() != p_parameters.value(ENCODER_WAVE_SUFFIX_KEY, ENCODER_WAVE_SUFFIX).toString());

    Q_EMIT triggerChanged();
}
