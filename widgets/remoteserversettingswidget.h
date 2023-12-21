/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef REMOTESERVERSETTINGSWIDGET_H
#define REMOTESERVERSETTINGSWIDGET_H

#include <QDir>
#include <QWidget>

#include "ui_remoteserversettingswidgetUI.h"

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

#endif
