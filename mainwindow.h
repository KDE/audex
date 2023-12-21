/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QDockWidget>
#include <QInputDialog>
#include <QLabel>
#include <QObject>
#include <QTreeView>

#include <KActionCollection>
#include <KCModuleLoader>
#include <KCDDB/CDInfo>
#include <KCDDB/Client>
#include <KCDDB/KCDDB>
#include <KComboBox>
#include <KConfigDialog>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>
#include <KTextEdit>
#include <KXmlGuiWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QStatusBar>

#include "utils/cuesheetwriter.h"
#include "utils/error.h"

#include "models/cddamodel.h"
#include "models/profilemodel.h"

#include "preferences.h"
#include "widgets/cddaheaderwidget.h"
#include "widgets/devicewidget.h"
#include "widgets/generalsettingswidget.h"
#include "widgets/profilewidget.h"
#include "widgets/remoteserversettingswidget.h"

#include "dialogs/errordialog.h"
#include "dialogs/extractingprogressdialog.h"

#include "utils/encoderassistant.h"

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    bool firstStart();

private Q_SLOTS:
    void eject();
    void cddb_lookup();
    void cddb_submit();
    void rip();
    void configure();

    void new_audio_disc_detected();
    void audio_disc_removed();

    void cddb_lookup_start();
    void cddb_lookup_done(const bool successful);

    void update_layout();

    void enable_layout(bool enabled);
    void enable_submit(bool enabled = true);
    void disable_submit();

    void configuration_updated(const QString &dialog_name);

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

    void cdda_context_menu(const QPoint &pos);

    void selection_changed(const int num_selected);

private:
    CDDAModel *cdda_model;
    ProfileModel *profile_model;

    QLabel *profile_label;
    KComboBox *profile_combobox;

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
