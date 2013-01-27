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

#ifndef WAVEWIDGET_H
#define WAVEWIDGET_H

#include <QWidget>
#include <QDir>

#include <KDebug>
#include <KDialog>

#include "utils/error.h"
#include "utils/parameters.h"
#include "utils/encoderassistant.h"

#include "ui_wavewidgetUI.h"

class waveWidgetUI : public QWidget, public Ui::WAVEWidgetUI {
public:
  explicit waveWidgetUI(QWidget *parent) : QWidget(parent) {
    setupUi(this);
  }
};

class waveWidget : public waveWidgetUI {
  Q_OBJECT
public:
  explicit waveWidget(Parameters *parameters, QWidget *parent = 0);
  ~waveWidget();
  Error lastError() const { return error; }
  inline bool isChanged() const { return changed; }
public slots:
  bool save();
signals:
  void triggerChanged();
private slots:
  void trigger_changed();
private:
  Parameters *parameters;
  Error error;
  bool changed;
};

#endif
