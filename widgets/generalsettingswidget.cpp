/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2013 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "generalsettingswidget.h"

generalSettingsWidget::generalSettingsWidget(QWidget* parent) : generalSettingsWidgetUI(parent) {

  kcfg_wikipediaLocale->clear();
  kcfg_wikipediaLocale->addItem(QIcon(KStandardDirs::locate("data", "audex/images/usa.png")), i18n("English"), "en");
  kcfg_wikipediaLocale->addItem(QIcon(KStandardDirs::locate("data", "audex/images/germany.png")), i18n("Deutsch"), "de");
  kcfg_wikipediaLocale->addItem(QIcon(KStandardDirs::locate("data", "audex/images/france.png")), i18n("Français"), "fr");
  kcfg_wikipediaLocale->addItem(QIcon(KStandardDirs::locate("data", "audex/images/poland.png")), i18n("Polski"), "pl");
  kcfg_wikipediaLocale->addItem(QIcon(KStandardDirs::locate("data", "audex/images/japan.png")), i18n("日本語"), "ja");
  kcfg_wikipediaLocale->addItem(QIcon(KStandardDirs::locate("data", "audex/images/italy.png")), i18n("Italiano"), "it");
  kcfg_wikipediaLocale->addItem(QIcon(KStandardDirs::locate("data", "audex/images/netherlands.png")), i18n("Nederlands"), "nl");
  kcfg_wikipediaLocale->addItem(QIcon(KStandardDirs::locate("data", "audex/images/spain.png")), i18n("Español"), "es");
  kcfg_wikipediaLocale->addItem(QIcon(KStandardDirs::locate("data", "audex/images/portugal.png")), i18n("Português"), "pt");
  kcfg_wikipediaLocale->addItem(QIcon(KStandardDirs::locate("data", "audex/images/sweden.png")), i18n("Svenska"), "sv");

  urlreq_basePath->setMode(KFile::Directory|KFile::LocalOnly);
  urlreq_basePath->lineEdit()->setObjectName("kcfg_basePath");

  QStringList devices = KCompactDisc::cdromDeviceNames();
  kcfg_cdDevice->clear();

  if (devices.isEmpty()) {
    kcfg_cdDevice->addItem(i18n("None detected"));
  } else {
    foreach (const QString &dev, devices) {
      kcfg_cdDevice->addItem(dev + " (" + KCompactDisc::cdromDeviceUrl(dev).path() + ")");
    }
  }
}

generalSettingsWidget::~generalSettingsWidget() {

}
