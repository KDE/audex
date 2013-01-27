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

#ifndef FLACWIDGET_H
#define FLACWIDGET_H

#include <QWidget>
#include <QDir>

#include <KDebug>
#include <KDialog>

#include "utils/error.h"
#include "utils/parameters.h"
#include "utils/encoderassistant.h"

#include "ui_flacwidgetUI.h"

class flacWidgetUI : public QWidget, public Ui::FLACWidgetUI {
public:
  explicit flacWidgetUI(QWidget *parent) : QWidget(parent) {
    setupUi(this);
  }
};

class flacWidget : public flacWidgetUI {
  Q_OBJECT
public:
  explicit flacWidget(Parameters *parameters, QWidget *parent = 0);
  ~flacWidget();
  inline Error lastError() const { return error; }
  inline bool isChanged() const { return changed; }
public slots:
  bool save();
signals:
  void triggerChanged();
private slots:
  void compression_changed_by_slider(int compression);
  void compression_changed_by_spinbox(int compression);
  void trigger_changed();
private:
  Parameters *parameters;
  Error error;
  bool changed;
};

#endif
