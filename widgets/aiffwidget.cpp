/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "aiffwidget.h"

#include "utils/encoderassistant.h"
#include "utils/parameters.h"

#include <QAbstractButton>
#include <QLineEdit>

aiffWidget::aiffWidget(const Parameters &parameters, QWidget *parent)
    : aiffWidgetUI(parent)
    , p_parameters(parameters)
{
    checkBox_embedcover->setChecked(p_parameters.value(ENCODER_AIFF_EMBED_COVER_KEY, ENCODER_AIFF_EMBED_COVER).toBool());
    qlineedit_suffix->setText(p_parameters.value(ENCODER_AIFF_SUFFIX_KEY, ENCODER_AIFF_SUFFIX).toString());

    connect(checkBox_embedcover, &QAbstractButton::toggled, this, &aiffWidget::trigger_changed);
    connect(qlineedit_suffix, &QLineEdit::textEdited, this, &aiffWidget::trigger_changed);

    changed = false;
}

aiffWidget::~aiffWidget()
{
}

bool aiffWidget::save()
{
    p_parameters.setValue(ENCODER_AIFF_EMBED_COVER_KEY, checkBox_embedcover->isChecked());
    p_parameters.setValue(ENCODER_AIFF_SUFFIX_KEY, qlineedit_suffix->text());
    changed = false;
    return true;
}

void aiffWidget::trigger_changed()
{
    changed = (checkBox_embedcover->isChecked() != p_parameters.value(ENCODER_AIFF_EMBED_COVER_KEY, ENCODER_AIFF_EMBED_COVER).toBool()
               || qlineedit_suffix->text() != p_parameters.value(ENCODER_AIFF_SUFFIX_KEY, ENCODER_AIFF_SUFFIX).toString());

    Q_EMIT triggerChanged();
}
