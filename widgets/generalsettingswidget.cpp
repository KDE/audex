/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "generalsettingswidget.h"

#include <KLocalizedString>
#include <QStandardPaths>

generalSettingsWidget::generalSettingsWidget(QWidget *parent)
    : generalSettingsWidgetUI(parent)
{
    kcfg_wikipediaLocale->clear();
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/usa.png")), i18n("English"), "en");
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/germany.png")), i18n("Deutsch"), "de");
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/france.png")), i18n("Français"), "fr");
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/poland.png")), i18n("Polski"), "pl");
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/japan.png")), i18n("日本語"), "ja");
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/italy.png")), i18n("Italiano"), "it");
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/netherlands.png")), i18n("Nederlands"), "nl");
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/spain.png")), i18n("Español"), "es");
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/portugal.png")), i18n("Português"), "pt");
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/sweden.png")), i18n("Svenska"), "sv");
    kcfg_wikipediaLocale->addItem(QIcon(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "audex/images/catalonia.png")), i18n("Català"), "ca");

    urlreq_basePath->setMode(KFile::Directory | KFile::LocalOnly);
    urlreq_basePath->lineEdit()->setObjectName("kcfg_basePath");
}

generalSettingsWidget::~generalSettingsWidget()
{
}
