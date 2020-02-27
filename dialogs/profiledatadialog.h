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

#ifndef PROFILEDATADIALOG_H
#define PROFILEDATADIALOG_H

#include <QWidget>
#include <QStackedWidget>

#include <QDialog>
#include <QPushButton>

#include "models/profilemodel.h"
#include "utils/encoderassistant.h"

#include "widgets/lamewidget.h"
#include "widgets/oggencwidget.h"
#include "widgets/flacwidget.h"
#include "widgets/faacwidget.h"
#include "widgets/wavewidget.h"
#include "widgets/customwidget.h"

#include "dialogs/profiledatacoverdialog.h"
#include "dialogs/profiledataplaylistdialog.h"
#include "dialogs/profiledatainfodialog.h"
#include "dialogs/profiledatahashlistdialog.h"
#include "dialogs/profiledatacuesheetdialog.h"
#include "dialogs/profiledatasinglefiledialog.h"
#include "dialogs/patternwizarddialog.h"
#include "dialogs/errordialog.h"

#include "ui_profiledatawidgetUI.h"

class ProfileDataDialog : public QDialog
{

  Q_OBJECT

public:
  ProfileDataDialog(ProfileModel *profileModel, const int profileRow, QWidget *parent = 0);
  ~ProfileDataDialog();

private slots:
  void set_encoder(const int encoder);
  void set_encoder_by_combobox(const int index);
  void trigger_changed();

  void enable_settings_cover(bool enabled);
  void enable_settings_playlist(bool enabled);
  void enable_settings_info(bool enabled);
  void enable_settings_hashlist(bool enabled);
  void enable_settings_cuesheet(bool enabled);
  void enable_settings_singlefile(bool enabled);

  void disable_playlist(bool disabled);

  void enable_filenames(bool enabled);
  void disable_filenames(bool disabled);

  void pattern_wizard();

  void cover_settings();
  void playlist_settings();
  void info_settings();
  void hashlist_settings();
  void cuesheet_settings();
  void singlefile_settings();

  void slotAccepted();
  void slotApplied();

private:
  Ui::ProfileDataWidgetUI ui;
  ProfileModel *profile_model;
  QPushButton *applyButton;

  int profile_row;

  lameWidget *lame_widget;
  Parameters lame_parameters;
  oggencWidget *oggenc_widget;
  Parameters oggenc_parameters;
  flacWidget *flac_widget;
  Parameters flac_parameters;
  faacWidget *faac_widget;
  Parameters faac_parameters;
  waveWidget *wave_widget;
  Parameters wave_parameters;
  customWidget *custom_widget;
  Parameters custom_parameters;
  void set_encoder_widget(const EncoderAssistant::Encoder encoder);

  bool pdcd_scale;
  QSize pdcd_size;
  QString pdcd_format;
  QString pdcd_pattern;

  QString pdpd_format;
  QString pdpd_pattern;
  bool pdpd_abs_file_path;
  bool pdpd_utf8;

  QStringList pdid_text;
  QString pdid_pattern;
  QString pdid_suffix;

  QString pdhd_format;
  QString pdhd_pattern;

  QString pdud_pattern;

  QString pdsd_pattern;

  bool save();

  Error error;

};

#endif
