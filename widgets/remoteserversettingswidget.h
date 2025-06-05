/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDir>
#include <QWidget>

#include "ui_remoteserversettingswidgetUI.h"

namespace Audex
{

class remoteServerSettingsWidgetUI : public QWidget, public Ui::RemoteServerSettingsWidgetUI
{
public:
    explicit remoteServerSettingsWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class remoteServerSettingsWidget : public remoteServerSettingsWidgetUI
{
    Q_OBJECT
public:
    explicit remoteServerSettingsWidget(QWidget *parent = nullptr);
    ~remoteServerSettingsWidget() override;
private Q_SLOTS:
    void toggle_upload(bool enabled);
};

}
