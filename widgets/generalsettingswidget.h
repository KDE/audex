/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDir>

#include <KLineEdit>

#include "preferences.h"

#include "ui_generalsettingswidgetUI.h"

namespace Audex
{

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

}
