/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <http://sourceforge.net/projects/audex>
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

#ifndef PROFILEDATAPLAYLISTDIALOG_H
#define PROFILEDATAPLAYLISTDIALOG_H

#include <QWidget>

#include <KDebug>
#include <KDialog>

#include "simplepatternwizarddialog.h"

#include "ui_profiledataplaylistwidgetUI.h"

class ProfileDataPlaylistDialog : public KDialog
{

  Q_OBJECT

public:
  ProfileDataPlaylistDialog(const QString& format, const QString& pattern, const bool absFilePath, const bool utf8, QWidget *parent = 0);
  ~ProfileDataPlaylistDialog();

  QString format;
  QString pattern;
  bool absFilePath;
  bool utf8;

protected slots:
  virtual void slotButtonClicked(int button);

  void pattern_wizard();

private slots:
  void trigger_changed();
  void enable_abs_file_path(bool enabled);
  void enable_utf8(bool enabled);

private:
  Ui::ProfileDataPlaylistWidgetUI ui;

  bool save();

};

#endif
