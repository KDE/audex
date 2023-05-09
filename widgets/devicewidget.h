/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QDir>

#include <QLineEdit>

#include "preferences.h"

#include "ui_devicewidgetUI.h"

class deviceWidgetUI : public QWidget, public Ui::DeviceWidgetUI
{
public:
    explicit deviceWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class deviceWidget : public deviceWidgetUI
{
    Q_OBJECT
public:
    explicit deviceWidget(QWidget *parent = nullptr);
    ~deviceWidget() override;
};

#endif
