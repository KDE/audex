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

#ifndef PATTERNWIZARDDIALOG_H
#define PATTERNWIZARDDIALOG_H

#include <QWidget>
#include <QWhatsThis>

#include <KDebug>
#include <KDialog>

#include "utils/patternparser.h"

#include "ui_patternwizardwidgetUI.h"

class PatternWizardDialog : public KDialog {

  Q_OBJECT

public:
  PatternWizardDialog(const QString& pattern, QWidget *parent = 0);
  ~PatternWizardDialog();

  QString pattern;

protected slots:
  virtual void slotButtonClicked(int button);

private slots:
  void trigger_changed();

  void about_filename_schemes();
  void about_parameters();
  
  void insAlbumArtist();
  void insAlbumTitle();
  void insTrackArtist();
  void insTrackTitle();
  void insTrackNo();
  void insCDNo();
  void insDate();
  void insGenre();
  void insSuffix();
  void insNoOfTracks();

  void update_example();
  
private:
  Ui::PatternWizardWidgetUI ui;

  bool save();

};

#endif
