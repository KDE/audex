/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <http://userbase.kde.org/Audex>
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

#ifndef DEVICEWIDGET_H
#define DEVICEWIDGET_H

#include <QDir>

#include <KLineEdit>
#include <KCompactDisc/kcompactdisc/kcompactdisc.h>

#include "preferences.h"

#include "ui_devicewidgetUI.h"

class deviceWidgetUI : public QWidget, public Ui::DeviceWidgetUI {
public:
  explicit deviceWidgetUI(QWidget *parent) : QWidget(parent) {
    setupUi(this);
  }
};

class deviceWidget : public deviceWidgetUI {
  Q_OBJECT
public:
  explicit deviceWidget(QWidget* parent = 0);
  ~deviceWidget();
};

#endif
