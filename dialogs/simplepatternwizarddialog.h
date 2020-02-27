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

#ifndef SIMPLEPATTERNWIZARDDIALOG_H
#define SIMPLEPATTERNWIZARDDIALOG_H

#include <QWidget>
#include <QWhatsThis>

#include <QDialog>

#include "utils/patternparser.h"

#include "ui_simplepatternwizardwidgetUI.h"

class SimplePatternWizardDialog : public QDialog {

  Q_OBJECT

public:
  SimplePatternWizardDialog(const QString& pattern, const QString& suffix, QWidget *parent = 0);
  ~SimplePatternWizardDialog();

  QString pattern;

private slots:
  void trigger_changed();
  
  void about_schemes();
  void about_parameters();

  void insAlbumArtist();
  void insAlbumTitle();
  void insCDNo();
  void insDate();
  void insGenre();
  void insSuffix();
  void insNoOfTracks();

  void update_example();

  void slotAccepted();
  void slotApplied();

private:
  Ui::SimplePatternWizardWidgetUI ui;
  QString suffix;
  QPushButton *applyButton;

  bool save();

};

#endif
