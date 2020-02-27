/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
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

#ifndef PROFILEDATACOVERDIALOG_H
#define PROFILEDATACOVERDIALOG_H

#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "simplepatternwizarddialog.h"

#include "ui_profiledatacoverwidgetUI.h"

class ProfileDataCoverDialog : public QDialog {

  Q_OBJECT

public:
  ProfileDataCoverDialog(const bool scale, const QSize& size, const QString& format, const QString& pattern, QWidget *parent = 0);
  ~ProfileDataCoverDialog();

  bool scale;
  QSize size;
  QString format;
  QString pattern;

protected slots:
  void pattern_wizard();

private slots:
  void trigger_changed();
  void enable_scale(bool enabled);

  void slotAccepted();
  void slotApplied();

private:
  Ui::ProfileDataCoverWidgetUI ui;
  QPushButton *applyButton;

  bool save();

};

#endif
