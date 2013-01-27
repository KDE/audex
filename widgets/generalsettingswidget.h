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

#ifndef GENERALSETTINGSWIDGET_H
#define GENERALSETTINGSWIDGET_H

#include <QDir>

#include <KLineEdit>
#include <KStandardDirs>
#include <libkcompactdisc/kcompactdisc.h>

#include "preferences.h"

#include "ui_generalsettingswidgetUI.h"

class generalSettingsWidgetUI : public QWidget, public Ui::GeneralSettingsWidgetUI {
public:
  explicit generalSettingsWidgetUI(QWidget *parent) : QWidget(parent) {
    setupUi(this);
  }
};

class generalSettingsWidget : public generalSettingsWidgetUI {
  Q_OBJECT
public:
  explicit generalSettingsWidget(QWidget* parent = 0);
  ~generalSettingsWidget();
};

#endif
