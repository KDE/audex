/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QComboBox>
#include <QDockWidget>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QStatusBar>
#include <QTreeView>
#include <QWidgetAction>

#include <KActionCollection>
#include <KCDDB/CDInfo>
#include <KCDDB/Client>
#include <KCDDB/KCDDB>
#include <KCModuleLoader>
#include <KComboBox>
#include <KConfigDialog>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>
#include <KTextEdit>
#include <KXmlGuiWindow>

#include "device/manager.h"

#include "datatypes/error.h"

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

private:
    bool firstStart();

private Q_SLOTS:
    void eject();
    void cddb_lookup();
    void cdtext_read();

    void rip();
    void configure();
    void edit();

    void update_layout();

    void enable_layout(bool enabled);

    void configuration_updated(const QString &dialog_name);

    void update_disc_action();

    void current_profile_updated_from_ui(int row);
    void update_profile_action_by_index(int index);
    void update_profile_action();

    void split_titles();
    void swap_artists_and_titles();
    void capitalize();
    void auto_fill_artists();
    void resizeColumns();

private:
    Audex::Device::Manager manager;
    Audex::CDDAModel cdda_model;
    Audex::ProfileModel profile_model;

    QPointer<QLabel> disc_label;
    QPointer<KComboBox> disc_combobox;
    QPointer<QLabel> profile_label;
    QPointer<KComboBox> profile_combobox;

    void setup_actions();
    void setup_layout();

    QPointer<QTreeView> cdda_tree_view;

    QPointer<QDockWidget> cdda_header_dock;
    QPointer<Audex::CDDAHeaderWidget> cdda_header_widget;

    bool layout_enabled;

    int current_profile_index;
    void set_profile(int profile_index);
};
