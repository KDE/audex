/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDir>

#include <QLineEdit>

#include "preferences.h"

#include "ui_devicewidgetUI.h"

namespace Audex
{

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

}
