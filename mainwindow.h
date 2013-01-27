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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QLabel>
#include <QComboBox>
#include <QTreeView>
#include <QDockWidget>
#include <QInputDialog>

#include <KXmlGuiWindow>
#include <KApplication>
#include <KAction>
#include <KLocale>
#include <KStandardDirs>
#include <KActionCollection>
#include <KStandardAction>
#include <KStatusBar>
#include <KConfigDialog>
#include <KCModuleLoader>
#include <KLineEdit>
#include <KComboBox>
#include <KTextEdit>
#include <KPushButton>
#include <KMessageBox>
#include <KMenu>
#include <libkcompactdisc/kcompactdisc.h>
#include <libkcddb/kcddb.h>
#include <libkcddb/client.h>
#include <libkcddb/cdinfo.h>

#include "utils/error.h"
#include "utils/cuesheetwriter.h"

#include "models/cddamodel.h"
#include "models/profilemodel.h"

#include "preferences.h"
#include "widgets/generalsettingswidget.h"
#include "widgets/profilewidget.h"
#include "widgets/remoteserversettingswidget.h"
#include "widgets/cddaheaderwidget.h"

#include "dialogs/extractingprogressdialog.h"

#include "utils/encoderassistant.h"

class MainWindow : public KXmlGuiWindow {

  Q_OBJECT

public:
  MainWindow(QWidget *parent = 0);
  ~MainWindow();

private:
  bool firstStart();

private Q_SLOTS:
  void eject();
  void cddb_lookup();
  void cddb_submit();
  void rip();
  void configure();

  void drive_status_changed(const CDDAModel::DriveStatus status);
  void disc_status_changed(const CDDAModel::DiscStatus status);
  void disc_changed(const CDDAModel::DiscType type);
  void disc_info_changed(const CDDAModel::DiscInfo info);

  void cddb_lookup_start();
  void cddb_lookup_done(const bool successful);

  void update_layout();

  void enable_layout(bool enabled);
  void enable_submit(bool enabled = TRUE);
  void disable_submit();

  void configuration_updated(const QString& dialog_name);

  void current_profile_updated_from_ui(int row);
  void update_profile_action(int index);
  void update_profile_action();

  void split_titles();
  void swap_artists_and_titles();
  void capitalize();
  void auto_fill_artists();
  void toggle(const QModelIndex &idx);
  void resizeColumns();

  void select_all();
  void select_none();
  void invert_selection();

  void cdda_context_menu(const QPoint& pos);

  void selection_changed(const int num_selected);

private:
  CDDAModel *cdda_model;
  ProfileModel *profile_model;

  QLabel *profile_label;
  KComboBox *profile_combobox;
  QLabel *status_label;
  QString status_label_prev;

  void setup_actions();
  void setup_layout();

  QTreeView *cdda_tree_view;

  QDockWidget *cdda_header_dock;
  CDDAHeaderWidget *cdda_header_widget;

  bool layout_enabled;

  int current_profile_index;
  void set_profile(int profile_index);

};

#endif
