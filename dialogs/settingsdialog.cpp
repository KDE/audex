/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "settingsdialog.h"

#include "widgets/devicewidget.h"

SettingsDialog::SettingsDialog(QWidget *parent, const QString &name, KCoreConfigSkeleton *config)
    : KConfigDialog(parent, name, config)
{
}

KPageWidgetItem *SettingsDialog::addDevicePage(deviceWidget *page, const QString &itemName)
{
    m_devicePage = page;
    connect(page, &deviceWidget::changed, this, &SettingsDialog::updateButtons);
    return addPage(page, itemName);
}

bool SettingsDialog::hasChanged()
{
    return m_devicePage && m_devicePage->hasChanged();
}

bool SettingsDialog::isDefault()
{
    return !m_devicePage || m_devicePage->isDefault();
}

void SettingsDialog::updateSettings()
{
    if (m_devicePage)
        m_devicePage->saveSettings();
}

void SettingsDialog::updateWidgets()
{
    if (m_devicePage)
        m_devicePage->loadSettings();
}

void SettingsDialog::updateWidgetsDefault()
{
    if (m_devicePage)
        m_devicePage->setDefaults();
}
