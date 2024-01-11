/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "devicewidget.h"

deviceWidget::deviceWidget(QWidget *parent)
    : deviceWidgetUI(parent)
{
    Q_UNUSED(parent);
}

deviceWidget::~deviceWidget()
{
}

void deviceWidget::setDeviceInfo(const QString &vendor, const QString &model, const QString &revision)
{
    label_Vendor->setText(vendor);
    label_Model->setText(model);
    label_Revision->setText(revision);
    label_Vendor->setEnabled(true);
    label_Model->setEnabled(true);
    label_Revision->setEnabled(true);
}

void deviceWidget::clearDeviceInfo()
{
    label_Vendor->setText(i18n("(No disc detected)"));
    label_Model->setText(i18n("(No disc detected)"));
    label_Revision->setText(i18n("(No disc detected)"));
    label_Vendor->setEnabled(false);
    label_Model->setEnabled(false);
    label_Revision->setEnabled(false);
}
