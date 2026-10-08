/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_generalsettingswidgetUI.h"

#include <QWidget>

class generalSettingsWidgetUI : public QWidget, public Ui::GeneralSettingsWidgetUI
{
public:
    explicit generalSettingsWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class generalSettingsWidget : public generalSettingsWidgetUI
{
    Q_OBJECT
public:
    explicit generalSettingsWidget(QWidget *parent = nullptr);
    ~generalSettingsWidget() override;
};
