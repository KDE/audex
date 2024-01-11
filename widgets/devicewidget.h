/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QDir>

#include <QLineEdit>

#include "utils/cddaparanoia.h"

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
public Q_SLOTS:
    void setDeviceInfo(const QString &vendor, const QString &model, const QString &revision);
    void clearDeviceInfo();
};

#endif
