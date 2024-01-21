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

void deviceWidget::setDeviceInfo(const QString &vendor,
                                 const QString &model,
                                 const QString &revision,
                                 const bool canReadMCN,
                                 const bool canReadISRC,
                                 const bool c2ErrorCorrection)
{
    label_Vendor->setText(vendor);
    label_Model->setText(model);
    label_Revision->setText(revision);

    label_Vendor->setEnabled(true);
    label_Model->setEnabled(true);
    label_Revision->setEnabled(true);

    checkBox_canReadMCN->setChecked(canReadMCN);
    checkBox_canReadISRC->setChecked(canReadISRC);
    checkBox_c2ErrorCorrection->setChecked(c2ErrorCorrection);

    checkBox_canReadMCN->setEnabled(true);
    checkBox_canReadISRC->setEnabled(true);
    checkBox_c2ErrorCorrection->setEnabled(true);
    checkBox_canReadMCN->setAttribute(Qt::WA_TransparentForMouseEvents);
    checkBox_canReadMCN->setFocusPolicy(Qt::NoFocus);
    checkBox_canReadISRC->setAttribute(Qt::WA_TransparentForMouseEvents);
    checkBox_canReadISRC->setFocusPolicy(Qt::NoFocus);
    checkBox_c2ErrorCorrection->setAttribute(Qt::WA_TransparentForMouseEvents);
    checkBox_c2ErrorCorrection->setFocusPolicy(Qt::NoFocus);
}

void deviceWidget::clearDeviceInfo()
{
    label_Vendor->setText(i18n("(No disc detected)"));
    label_Model->setText(i18n("(No disc detected)"));
    label_Revision->setText(i18n("(No disc detected)"));
    label_Vendor->setEnabled(false);
    label_Model->setEnabled(false);
    label_Revision->setEnabled(false);
    checkBox_canReadMCN->setChecked(false);
    checkBox_canReadISRC->setChecked(false);
    checkBox_c2ErrorCorrection->setChecked(false);
    checkBox_canReadMCN->setEnabled(false);
    checkBox_canReadISRC->setEnabled(false);
    checkBox_c2ErrorCorrection->setEnabled(false);
}
