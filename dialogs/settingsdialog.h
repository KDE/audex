/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <KConfigDialog>
#include <KPageWidgetItem>
#include <QPointer>

class deviceWidget;

// KConfigDialog only tracks the widgets managed by KConfigXT. The device
// page stores its values per drive, so its state is added here: Apply,
// OK, Cancel and Defaults cover it like any other page.
class SettingsDialog : public KConfigDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent, const QString &name, KCoreConfigSkeleton *config);

    KPageWidgetItem *addDevicePage(deviceWidget *page, const QString &itemName);

protected:
    bool hasChanged() override;
    bool isDefault() override;
    void updateSettings() override;
    void updateWidgets() override;
    void updateWidgetsDefault() override;

private:
    QPointer<deviceWidget> m_devicePage;
};
