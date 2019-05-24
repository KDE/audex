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

#ifndef CDDAHEADERDATADIALOG_H
#define CDDAHEADERDATADIALOG_H

#include <QWidget>
#include <QDate>
#include <QDialog>
#include <QPushButton>

#include "models/cddamodel.h"

#include "utils/discidcalculator.h"

#include "ui_cddaheaderdatawidgetUI.h"

class CDDAHeaderDataDialog : public QDialog {

  Q_OBJECT

public:
  explicit CDDAHeaderDataDialog(CDDAModel *cddaModel, QWidget *parent = 0);
  ~CDDAHeaderDataDialog();

private slots:
  void save();
  void trigger_changed();
  void enable_checkbox_multicd(bool enabled);

  void slotAccepted();
  void slotApplied();

private:
  Ui::CDDAHeaderDataWidgetUI ui;
  CDDAModel *cdda_model;
  QPushButton *okButton;
  QPushButton *applyButton;

};

#endif
