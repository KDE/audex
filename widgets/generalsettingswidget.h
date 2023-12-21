/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GENERALSETTINGSWIDGET_H
#define GENERALSETTINGSWIDGET_H

#include <QDir>

#include <KLineEdit>

#include "preferences.h"

#include "ui_generalsettingswidgetUI.h"

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

#endif
