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

#include "mainwindow.h"
#include "utils/errordialog.h"

MainWindow::MainWindow(QWidget *parent) : KXmlGuiWindow(parent) {

  profile_model = new ProfileModel(this);
  if (!profile_model) {
    kDebug() << "Unable to create ProfileModel object. Low mem?";
    ErrorDialog::show(this, i18n("Unable to create ProfileModel object."), i18n("Internal error. Check your hardware. If all okay please make bug report."));
    return;
  }
  if (profile_model->lastError().isValid()) {
    ErrorDialog::show(this, profile_model->lastError().message(), profile_model->lastError().details());
    return;
  }

  bool updated = firstStart();

  cdda_model = new CDDAModel(this, KCompactDisc::cdromDeviceUrl(Preferences::cdDevice()).path());
  if (!cdda_model) {
    kDebug() << "Unable to create CDDAModel object. Low mem?";
    ErrorDialog::show(this, i18n("Unable to create CDDAModel object."), i18n("Internal error. Check your hardware. If all okay please make bug report."));
    return;
  }
  if (cdda_model->lastError().isValid()) {
    ErrorDialog::show(this, cdda_model->lastError().message(), cdda_model->lastError().details());
    return;
  }

  connect(cdda_model, SIGNAL(driveStatusChanged(const CDDAModel::DriveStatus)), this, SLOT(drive_status_changed(const CDDAModel::DriveStatus)));
  connect(cdda_model, SIGNAL(discStatusChanged(const CDDAModel::DiscStatus)), this, SLOT(disc_status_changed(const CDDAModel::DiscStatus)));
  connect(cdda_model, SIGNAL(discChanged(const CDDAModel::DiscType)), this, SLOT(disc_changed(const CDDAModel::DiscType)));
  connect(cdda_model, SIGNAL(discInfoChanged(const CDDAModel::DiscInfo)), this, SLOT(disc_info_changed(const CDDAModel::DiscInfo)));
  connect(cdda_model, SIGNAL(cddbLookupStarted()), this, SLOT(cddb_lookup_start()));
  connect(cdda_model, SIGNAL(cddbLookupDone(const bool)), this, SLOT(cddb_lookup_done(const bool)));
  connect(cdda_model, SIGNAL(cddbDataModified()), this, SLOT(enable_submit()));
  connect(cdda_model, SIGNAL(cddbDataModified()), this, SLOT(update_layout()));
  connect(cdda_model, SIGNAL(cddbDataSubmited(bool)), this, SLOT(enable_submit(bool)));

  connect(profile_model, SIGNAL(profilesRemovedOrInserted()), this, SLOT(update_profile_action()));
  connect(profile_model, SIGNAL(currentProfileIndexChanged(int)), this, SLOT(update_profile_action(int)));

  setup_actions();
  setup_layout();
  setupGUI();

  enable_layout(FALSE);

  if (updated) {
    update();
    resize(650, 500);
  }

}

bool MainWindow::firstStart() {

  if (Preferences::firstStart()) {
    profile_model->autoCreate();
    Preferences::setFirstStart(FALSE);
    Preferences::self()->writeConfig();
    return TRUE;
  }

  return FALSE;

}

MainWindow::~MainWindow() {

  delete profile_model;
  delete cdda_model;

}

void MainWindow::eject() {
  kDebug() << "eject requested";
  cdda_model->eject();
}

void MainWindow::cddb_lookup() {
  cdda_model->lookupCDDB();
}

void MainWindow::cddb_submit() {
  if (!cdda_model->submitCDDB()) {
    ErrorDialog::show(this, cdda_model->lastError().message(), cdda_model->lastError().details());
  }
}

void MainWindow::rip() {
  
  if (cdda_model->discInfo() == CDDAModel::DiscNoInfo) {

    if (KMessageBox::warningYesNo(this, i18n("No disc information set. Do you really want to continue?"),
				i18n("Disc information not found"),
				KStandardGuiItem::yes(),
				KStandardGuiItem::no(),
				"no_disc_info_warn")== KMessageBox::No) return;

  }

  if ((profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SF_INDEX)).toBool()) &&
      (cdda_model->numOfAudioTracksInSelection() < cdda_model->numOfAudioTracks())) {
    
    if (KMessageBox::warningYesNo(this, i18n("Single file rip selected but not all audio tracks to rip selected. Do you really want to continue?"),
				i18n("Not all audio tracks selected for single file rip"),
				KStandardGuiItem::yes(),
				KStandardGuiItem::no(),
				"singlefile_selection_warn")== KMessageBox::No) return;
    
  }

  ExtractingProgressDialog *dialog = new ExtractingProgressDialog(profile_model, cdda_model, this);

  dialog->setWindowModality(Qt::ApplicationModal);

  dialog->exec();

  delete dialog;

}

void MainWindow::configure() {

  if (KConfigDialog::showDialog("settings")) return;

  KConfigDialog* dialog = new KConfigDialog(this, "settings", Preferences::self());

  KPageWidgetItem *generalPage = dialog->addPage(new generalSettingsWidget(), i18n("General settings"));
  generalPage->setIcon(KIcon(KApplication::windowIcon()));

  KPageWidgetItem *profilePage = dialog->addPage(new profileWidget(profile_model), i18n("Profiles"));
  profilePage->setIcon(KIcon("document-multiple"));

  KService::Ptr libkcddb = KService::serviceByDesktopName("libkcddb");
  if (libkcddb && libkcddb->isValid()) {
    KCModuleInfo info(libkcddb->entryPath());
    if (info.service()->isValid()) {
      KCModule *m = KCModuleLoader::loadModule(info, KCModuleLoader::Inline);
      if (m) {
        m->load();
        KCDDB::Config* cfg = new KCDDB::Config();
        cfg->readConfig();
        dialog->addPage(m, cfg, QString("CDDB settings"), "text-xmcd");
        connect(dialog, SIGNAL(okClicked()), m, SLOT(save()));
        connect(dialog, SIGNAL(applyClicked()), m, SLOT(save()));
        connect(dialog, SIGNAL(defaultClicked()), m, SLOT(defaults()));
      }
    }
  }

  KPageWidgetItem *remoteServerPage = dialog->addPage(new remoteServerSettingsWidget(), i18n("Remote Server"));
  remoteServerPage->setIcon(KIcon("network-server"));

  connect(dialog, SIGNAL(settingsChanged(const QString&)), this, SLOT(configuration_updated(const QString&)));

  dialog->exec();

}

void MainWindow::drive_status_changed(const CDDAModel::DriveStatus status) {
  switch (status) {
    case CDDAModel::DriveNoStatus :
      status_label->setText(i18n("No status information available"));
      enable_layout(FALSE);
      break;
    case CDDAModel::DriveEmpty :
      status_label->setText(i18n("No disc in drive"));
      enable_layout(TRUE);
      break;
    case CDDAModel::DriveReady :
      status_label->setText(i18n("Audio disc in drive"));
      enable_layout(TRUE);
      resizeColumns();
      if (Preferences::cddbLookupAuto()) {
        kDebug() << "Performing CDDB auto lookup";
        QTimer::singleShot(0, this, SLOT(cddb_lookup()));
      }
      break;
    case CDDAModel::DriveOpen :
      status_label->setText(i18n("Drive tray open"));
      enable_layout(FALSE);
      break;
    case CDDAModel::DriveNotReady :
      status_label->setText(i18n("Drive not ready"));
      enable_layout(FALSE);
      break;
    case CDDAModel::DriveError :
    status_label->setText(i18n("Drive error"));
      enable_layout(FALSE);
      break;
    default :
      break;
  }
}

void MainWindow::disc_status_changed(const CDDAModel::DiscStatus status) {
  Q_UNUSED(status);
}

void MainWindow::disc_changed(const CDDAModel::DiscType type) {
  Q_UNUSED(type);
  update_layout();
}

void MainWindow::disc_info_changed(const CDDAModel::DiscInfo info) {
  Q_UNUSED(info);
  switch (info) {
    case CDDAModel::DiscNoInfo :
      break;
    case CDDAModel::DiscManualInfo :
      break;
    case CDDAModel::DiscCDTEXTInfo :
    case CDDAModel::DiscCDDBInfo :
    case CDDAModel::DiscPhononMetadataInfo :
      if (Preferences::coverLookupAuto()) cdda_header_widget->googleAuto();
      break;
    default :
      break;
  }
  update_layout();
}

void MainWindow::cddb_lookup_start() {
  status_label_prev = status_label->text();
  status_label->setText(i18n("Fetching CDDB information..."));
}

void MainWindow::cddb_lookup_done(const bool successful) {
  if (!successful) {
    ErrorDialog::show(this, i18n("CDDB lookup failed, with the following error:\n%1", cdda_model->lastError().message()),
                     cdda_model->lastError().details(), i18n("CDD Lookup Failure"));
  }
  status_label->setText(status_label_prev);
  update_layout();
  disable_submit();
}

void MainWindow::update_layout() {
  if (!cdda_model->isVarious()) {
    cdda_tree_view->hideColumn(CDDA_MODEL_COLUMN_ARTIST_INDEX);
  } else {
    cdda_tree_view->showColumn(CDDA_MODEL_COLUMN_ARTIST_INDEX);
  }
  resizeColumns();
  actionCollection()->action("selectall")->setEnabled(cdda_model->selectedTracks().count()<cdda_model->numOfAudioTracks());
  actionCollection()->action("selectnone")->setEnabled(cdda_model->selectedTracks().count()>0);
}

void MainWindow::enable_layout(bool enabled) {

  layout_enabled = enabled;
  cdda_tree_view->setEnabled(enabled);
  cdda_header_dock->setEnabled(enabled);
  cdda_header_widget->setEnabled(enabled);
  actionCollection()->action("profile_label")->setEnabled((profile_model->rowCount() > 0) && (enabled));
  profile_combobox->setEnabled((profile_model->rowCount() > 0) && (enabled));
  actionCollection()->action("profile")->setEnabled((profile_model->rowCount() > 0) && (enabled));
  actionCollection()->action("fetch")->setEnabled(enabled);
  if (cdda_model->isModified())
    actionCollection()->action("submit")->setEnabled(enabled);
  else
    actionCollection()->action("submit")->setEnabled(FALSE);
  actionCollection()->action("rip")->setEnabled(enabled);
  actionCollection()->action("splittitles")->setEnabled(enabled);
  actionCollection()->action("swapartistsandtitles")->setEnabled(enabled);
  actionCollection()->action("capitalize")->setEnabled(enabled);
  actionCollection()->action("autofillartists")->setEnabled(enabled);
  actionCollection()->action("selectall")->setEnabled(enabled);
  actionCollection()->action("selectnone")->setEnabled(enabled);
  actionCollection()->action("invertselection")->setEnabled(enabled);

}

void MainWindow::enable_submit(bool enabled) {
  actionCollection()->action("submit")->setEnabled(enabled);
}

void MainWindow::disable_submit() {
  actionCollection()->action("submit")->setEnabled(FALSE);
}

void MainWindow::configuration_updated(const QString& dialog_name) {
  Q_UNUSED(dialog_name);
  Preferences::self()->writeConfig();
  QString dev = KCompactDisc::cdromDeviceUrl(Preferences::cdDevice()).path();
  if (dev != cdda_model->device()) {
    cdda_model->setDevice(dev);
  }
}

void MainWindow::current_profile_updated_from_ui(int row) {
  if (row >= 0) {
    profile_model->blockSignals(TRUE);
    profile_model->setRowAsCurrentProfileIndex(row);
    profile_model->blockSignals(FALSE);
  }
}

void MainWindow::update_profile_action(int index) {

  if (index == -1) {
    if (layout_enabled) {
      actionCollection()->action("profile_label")->setEnabled(FALSE);
      actionCollection()->action("profile")->setEnabled(FALSE);
    }
  } else {
    if (layout_enabled) {
      actionCollection()->action("profile_label")->setEnabled(TRUE);
      actionCollection()->action("profile")->setEnabled(TRUE);
    }
    profile_combobox->setCurrentIndex(profile_model->getRowByIndex(index));
  }

}

void MainWindow::update_profile_action() {

  // When the Profile model emits 'reset' the profile combo clears its current settings.
  // Therefore, we need to try and reset these...
  if (profile_combobox->currentText().isEmpty()) {
    profile_combobox->setCurrentIndex(profile_model->currentProfileRow());
  }

  if (layout_enabled) {
    actionCollection()->action("profile_label")->setEnabled(profile_model->rowCount() > 0);
    actionCollection()->action("profile")->setEnabled(profile_model->rowCount() > 0);
  }

}

void MainWindow::split_titles() {

  bool ok;
  QString divider = QInputDialog::getText(this, i18n("Split titles"),
                                          i18n("Please set a divider string. Be aware of empty spaces.\n\nDivider:"), QLineEdit::Normal,
                                          " - ", &ok);
  if (ok && !divider.isEmpty()) {
    cdda_model->splitTitleOfTracks(divider);
  }

}

void MainWindow::swap_artists_and_titles() {

  if (KMessageBox::warningYesNo(this, i18n("Do you really want to swap all artists and titles?"),
				i18n("Swap artists and titles"),
				KStandardGuiItem::yes(),
				KStandardGuiItem::no(),
				"no_swap_artists_and_titles_warn")== KMessageBox::No) return;

  cdda_model->swapArtistAndTitle();
  cdda_model->swapArtistAndTitleOfTracks();

}

void MainWindow::capitalize() {

  if (KMessageBox::warningYesNo(this, i18n("Do you really want to capitalize all artists and titles?"),
				i18n("Capitalize artists and titles"),
				KStandardGuiItem::yes(),
				KStandardGuiItem::no(),
				"no_capitalize_warn")== KMessageBox::No) return;

  cdda_model->capitalizeHeader();
  cdda_model->capitalizeTracks();

}

void MainWindow::auto_fill_artists() {

  if (KMessageBox::warningYesNo(this, i18n("Do you really want to autofill track artists?"),
				i18n("Autofill artists"),
				KStandardGuiItem::yes(),
				KStandardGuiItem::no(),
				"no_autofill_warn")== KMessageBox::No) return;

  cdda_model->setTitleArtistsFromHeader();

}

void MainWindow::toggle(const QModelIndex &idx) {
  if (idx.isValid() && (idx.column() == CDDA_MODEL_COLUMN_RIP_INDEX)) {
    cdda_model->toggle(idx.row());
    cdda_tree_view->update(idx);
  }
}

void MainWindow::resizeColumns() {
  for (int i = 0; i < CDDA_MODEL_COLUMN_COUNT; ++i)
    cdda_tree_view->resizeColumnToContents(i);
}

void MainWindow::setup_actions() {

  KAction* ejectAction = new KAction(this);
  ejectAction->setText(i18n("Eject"));
  ejectAction->setIcon(KIcon("media-eject"));
  ejectAction->setShortcut(Qt::CTRL + Qt::Key_E);
  actionCollection()->addAction("eject", ejectAction);
  connect(ejectAction, SIGNAL(triggered(bool)), this, SLOT(eject()));

  profile_label = new QLabel(this);
  profile_label->setText(i18n("Profile:"));
  profile_combobox = new KComboBox(this);
  profile_combobox->setModel(profile_model);
  profile_combobox->setModelColumn(1);
  profile_combobox->setMinimumWidth(80);
  profile_combobox->setMaximumWidth(220);
  profile_combobox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  profile_combobox->resize(QSize(220, profile_combobox->height()));
  profile_combobox->setCurrentIndex(profile_model->currentProfileRow());
  connect(profile_combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(current_profile_updated_from_ui(int)));

  KAction *plabelAction = new KAction(this);
  plabelAction->setText(i18n("&Profile:"));
  plabelAction->setDefaultWidget(profile_label);
  profile_label->setBuddy(profile_combobox);
  actionCollection()->addAction("profile_label", plabelAction);

  KAction *profileAction = new KAction(this);
  profileAction->setText(i18n("Profile"));
  profileAction->setShortcut(Qt::Key_F6);
  profileAction->setDefaultWidget(profile_combobox);
  profileAction->setShortcutConfigurable(FALSE);
  actionCollection()->addAction("profile", profileAction);
  update_profile_action();

  KAction *cddbLookupAction = new KAction(this);
  cddbLookupAction->setText(i18n("Fetch Info"));
  cddbLookupAction->setIcon(KIcon("view-list-text"));
  cddbLookupAction->setShortcut(Qt::CTRL + Qt::Key_F);
  actionCollection()->addAction("fetch", cddbLookupAction);
  connect(cddbLookupAction, SIGNAL(triggered(bool)), this, SLOT(cddb_lookup()));

  KAction *cddbSubmitAction = new KAction(this);
  cddbSubmitAction->setText(i18n("Submit Info"));
  cddbSubmitAction->setShortcut(Qt::CTRL + Qt::Key_S);
  actionCollection()->addAction("submit", cddbSubmitAction);
  connect(cddbSubmitAction, SIGNAL(triggered(bool)), this, SLOT(cddb_submit()));

  KAction* extractAction = new KAction(this);
  extractAction->setText(i18n("Rip..."));
  extractAction->setIcon(KIcon("media-optical-audio"));
  extractAction->setShortcut(Qt::CTRL + Qt::Key_X);
  actionCollection()->addAction("rip", extractAction);
  connect(extractAction, SIGNAL(triggered(bool)), this, SLOT(rip()));

  actionCollection()->addAction("preferences", KStandardAction::preferences(this, SLOT(configure()), this));

  KAction *splitTitlesAction = new KAction(this);
  splitTitlesAction->setText(i18n("Split Titles..."));
  actionCollection()->addAction("splittitles", splitTitlesAction);
  connect(splitTitlesAction, SIGNAL(triggered(bool)), this, SLOT(split_titles()));

  KAction *swapArtistsAndTitlesAction = new KAction(this);
  swapArtistsAndTitlesAction->setText(i18n("Swap Artists And Titles"));
  actionCollection()->addAction("swapartistsandtitles", swapArtistsAndTitlesAction);
  connect(swapArtistsAndTitlesAction, SIGNAL(triggered(bool)), this, SLOT(swap_artists_and_titles()));

  KAction *capitalizeAction = new KAction(this);
  capitalizeAction->setText(i18n("Capitalize"));
  actionCollection()->addAction("capitalize", capitalizeAction);
  connect(capitalizeAction, SIGNAL(triggered(bool)), this, SLOT(capitalize()));

  KAction *autoFillArtistsAction = new KAction(this);
  autoFillArtistsAction->setText(i18n("Auto Fill Artists"));
  actionCollection()->addAction("autofillartists", autoFillArtistsAction);
  connect(autoFillArtistsAction, SIGNAL(triggered(bool)), this, SLOT(auto_fill_artists()));

  KAction *selectAllAction = new KAction(this);
  selectAllAction->setText(i18n("Select All Tracks"));
  actionCollection()->addAction("selectall", selectAllAction);
  connect(selectAllAction, SIGNAL(triggered(bool)), this, SLOT(select_all()));

  KAction *selectNoneAction = new KAction(this);
  selectNoneAction->setText(i18n("Deselect All Tracks"));
  actionCollection()->addAction("selectnone", selectNoneAction);
  connect(selectNoneAction, SIGNAL(triggered(bool)), this, SLOT(select_none()));

  KAction *invertSelectionAction = new KAction(this);
  invertSelectionAction->setText(i18n("Invert Selection"));
  actionCollection()->addAction("invertselection", invertSelectionAction);
  connect(invertSelectionAction, SIGNAL(triggered(bool)), this, SLOT(invert_selection()));

  KStandardAction::quit(kapp, SLOT(quit()), actionCollection());

}

void MainWindow::setup_layout() {

  cdda_tree_view = new QTreeView(this);
  cdda_tree_view->setModel(cdda_model);
  cdda_tree_view->setAlternatingRowColors(TRUE);
  cdda_tree_view->setSelectionBehavior(QAbstractItemView::SelectRows);
  cdda_tree_view->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);
  cdda_tree_view->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  cdda_tree_view->setIndentation(0);
  cdda_tree_view->setAllColumnsShowFocus(TRUE);
  cdda_tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(cdda_tree_view, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(cdda_context_menu(const QPoint&)));
  connect(cdda_tree_view, SIGNAL(clicked(const QModelIndex&)), SLOT(toggle(const QModelIndex&)));
  //connect(cdda_model, SIGNAL(discInfoChanged(const CDDAModel::DiscInfo)), SLOT(resizeColumns()));
  connect(cdda_model, SIGNAL(selectionChanged(const int)), this, SLOT(selection_changed(const int)));

  cdda_header_dock = new QDockWidget(this);
  cdda_header_dock->setObjectName("cdda_header_dock");
  cdda_header_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  cdda_header_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

  setCentralWidget(cdda_tree_view);
  cdda_header_widget = new CDDAHeaderWidget(cdda_model, cdda_header_dock);
  connect(cdda_header_widget, SIGNAL(headerDataChanged()), this, SLOT(update_layout()));
  cdda_header_dock->setWidget(cdda_header_widget);
  addDockWidget(Qt::LeftDockWidgetArea, cdda_header_dock);

  status_label = new QLabel();
  statusBar()->addWidget(status_label);
}

void MainWindow::select_all() {
  cdda_model->selectAll();
}

void MainWindow::select_none() {
  cdda_model->selectNone();
}

void MainWindow::invert_selection() {
  cdda_model->invertSelection();
}

void MainWindow::cdda_context_menu(const QPoint& pos) {
  Q_UNUSED(pos);
  KMenu menu(this);
  menu.addAction(actionCollection()->action("selectall"));
  menu.addAction(actionCollection()->action("selectnone"));
  menu.addSeparator();
  menu.addAction(actionCollection()->action("invertselection"));
  menu.exec(QCursor::pos());
}

void MainWindow::selection_changed(const int num_selected) {
  actionCollection()->action("rip")->setEnabled(num_selected>0);
  actionCollection()->action("selectall")->setEnabled(num_selected<cdda_model->numOfAudioTracks());
  actionCollection()->action("selectnone")->setEnabled(num_selected>0);
}
