/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "devicewidget.h"

namespace Audex
{

deviceWidget::deviceWidget(QWidget *parent)
    : deviceWidgetUI(parent)
{
    Q_UNUSED(parent);
}

deviceWidget::~deviceWidget()
{
}

}
