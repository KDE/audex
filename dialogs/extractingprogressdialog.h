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

#ifndef EXTRACTINGPROGRESSDIALOG_H
#define EXTRACTINGPROGRESSDIALOG_H

#include <QWidget>
#include <QDBusMessage>
#include <QDBusConnection>

#include <KDebug>
#include <KDialog>
#include <KMessageBox>
#include <KColorScheme>
#include <KConfigGroup>
#include <KGlobal>

#include "models/cddamodel.h"
#include "models/profilemodel.h"
#include "core/audex.h"

#include "protocoldialog.h"

#include "ui_extractingprogresswidgetUI.h"

class ExtractingProgressDialog : public KDialog {

  Q_OBJECT

public:
  ExtractingProgressDialog(ProfileModel *profile_model, CDDAModel *cdda_model, QWidget *parent = 0);
  ~ExtractingProgressDialog();

public slots:
  int exec();

private slots:
  void toggle_details();
  void slotButtonClicked(int button);
  void cancel();

  void show_changed_extract_track(int no, int total, const QString& artist, const QString& title);
  void show_changed_encode_track(int no, int total, const QString& filename);

  void show_progress_extract_track(int percent);
  void show_progress_extract_overall(int percent);
  void show_progress_encode_track(int percent);
  void show_progress_encode_overall(int percent);

  void show_speed_encode(double speed);
  void show_speed_extract(double speed);

  void conclusion(bool successful);

  void show_info(const QString& message);
  void show_warning(const QString& message);
  void show_error(const QString& message, const QString& details);

  void ask_timeout();

private:
  void calc_overall_progress();
  void open_encoder_protocol_dialog();
  void open_extract_protocol_dialog();
  void update_unity();

private:
  Ui::ExtractingProgressWidgetUI ui;

  Audex *audex;
  ProfileModel *profile_model;
  CDDAModel *cdda_model;

  bool finished;

  bool progressbar_np_flag;
  int current_encode_overall;
  int current_extract_overall;
  unsigned int current_track;
  
  bool p_single_file;
  
  QDBusMessage unity_message;
};

#endif
