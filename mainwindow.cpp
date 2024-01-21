/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mainwindow.h"
#include "widgets/devicewidget.h"

#include <QMenu>
#include <QWidgetAction>

class CDDATreeView : public QTreeView
{
public:
    CDDATreeView(QWidget *parent = nullptr)
        : QTreeView(parent)
    {
    }

protected:
    void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint) override
    {
        QTreeView::closeEditor(editor, hint);
        if ((this->currentIndex().row() < this->model()->rowCount() - 1) && (hint == QAbstractItemDelegate::SubmitModelCache)) {
            QTreeView::closeEditor(nullptr, QAbstractItemDelegate::EditNextItem);
        }
    }
};

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    profile_model = new ProfileModel(this);
    if (!profile_model) {
        qDebug() << "Unable to create ProfileModel object. Low mem?";
        ErrorDialog::show(this,
                          i18n("Unable to create ProfileModel object."),
                          i18n("Internal error. Check your hardware. If all okay please make bug report."));
        return;
    }
    if (profile_model->lastError().isValid()) {
        ErrorDialog::show(this, profile_model->lastError().message(), profile_model->lastError().details());
        return;
    }

    bool updated = firstStart();

    cdda_model = new CDDAModel(this);
    if (!cdda_model) {
        qDebug() << "Unable to create CDDAModel object. Low mem?";
        ErrorDialog::show(this, i18n("Unable to create CDDAModel object."), i18n("Internal error. Check your hardware. If all okay please make bug report."));
        return;
    }
    if (cdda_model->lastError().isValid()) {
        ErrorDialog::show(this, cdda_model->lastError().message(), cdda_model->lastError().details());
        return;
    }

    connect(cdda_model, SIGNAL(audioDiscDetected()), this, SLOT(new_audio_disc_detected()));
    connect(cdda_model, SIGNAL(audioDiscRemoved()), this, SLOT(audio_disc_removed()));

    connect(cdda_model, SIGNAL(cddbLookupStarted()), this, SLOT(cddb_lookup_start()));
    connect(cdda_model, SIGNAL(cddbLookupDone(const bool)), this, SLOT(cddb_lookup_done(const bool)));
    connect(cdda_model, SIGNAL(cddbDataModified()), this, SLOT(enable_cddb_submit()));
    connect(cdda_model, SIGNAL(cddbDataModified()), this, SLOT(update_layout()));
    connect(cdda_model, SIGNAL(cddbDataSubmited(bool)), this, SLOT(enable_cddb_submit(bool)));

    connect(profile_model, SIGNAL(profilesRemovedOrInserted()), this, SLOT(update_profile_action()));
    connect(profile_model, SIGNAL(currentProfileIndexChanged(int)), this, SLOT(update_profile_action(int)));

    setup_actions();
    setup_layout();
    setupGUI();

    enable_layout(false);

    if (updated) {
        update();
        resize(650, 500);
    }

    device_widget = nullptr;
}

bool MainWindow::firstStart()
{
    if (Preferences::firstStart()) {
        profile_model->autoCreate();
        Preferences::setFirstStart(false);
        Preferences::self()->save();
        return true;
    }

    return false;
}

MainWindow::~MainWindow()
{
    delete profile_model;
    delete cdda_model;
}

void MainWindow::eject()
{
    qDebug() << "eject requested";
    cdda_model->eject();
}

void MainWindow::cddb_lookup()
{
    cdda_model->lookupCDDB();
}

void MainWindow::cddb_submit()
{
    QStringList toc = cdda_model->cdio()->prettyTOC();
    for (int i = 0; i < toc.size(); ++i)
        qDebug() << toc.at(i);
    return;
    if (!cdda_model->submitCDDB()) {
        ErrorDialog::show(this, cdda_model->lastError().message(), cdda_model->lastError().details());
    }
}

void MainWindow::rip()
{
    if (cdda_model->empty()) {
        if (KMessageBox::warningTwoActions(this,
                                           i18n("No disc information set. Do you really want to continue?"),
                                           i18n("Disc information not found"),
                                           KStandardGuiItem::cont(),
                                           KStandardGuiItem::cancel(),
                                           "no_disc_info_warn")
            == KMessageBox::SecondaryAction)
            return;
    }

    if ((profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SF_INDEX)).toBool())
        && (cdda_model->numOfAudioTracksInSelection() < cdda_model->numOfAudioTracks())) {
        if (KMessageBox::warningTwoActions(this,
                                           i18n("Single file rip selected but not all audio tracks to rip selected. Do you really want to continue?"),
                                           i18n("Not all audio tracks selected for single file rip"),
                                           KStandardGuiItem::cont(),
                                           KStandardGuiItem::cancel(),
                                           "singlefile_selection_warn")
            == KMessageBox::SecondaryAction)
            return;
    }

    if ((profile_model->isSelectedEncoderWithEmbedCover()
         || (profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_INDEX)).toBool()))
        && cdda_model->isCoverEmpty()) {
        if (KMessageBox::warningTwoActions(this,
                                           i18n("No cover was set. Do you really want to continue?"),
                                           i18n("Cover is empty"),
                                           KStandardGuiItem::cont(),
                                           KStandardGuiItem::cancel(),
                                           "empty_cover_warn")
            == KMessageBox::SecondaryAction)
            return;
    }

    auto *dialog = new ExtractingProgressDialog(profile_model, cdda_model, this);

    dialog->setWindowModality(Qt::ApplicationModal);

    dialog->exec();

    delete dialog;
}

void MainWindow::configure()
{
    if (KConfigDialog::showDialog("settings"))
        return;

    KConfigDialog *dialog = new KConfigDialog(this, "settings", Preferences::self());

    KPageWidgetItem *generalPage = dialog->addPage(new generalSettingsWidget(), i18n("General settings"));
    generalPage->setIcon(QIcon(QApplication::windowIcon()));

    device_widget = new deviceWidget();
    KPageWidgetItem *devicePage = dialog->addPage(device_widget, i18n("Device settings"));
    devicePage->setIcon(QIcon::fromTheme("drive-optical"));

    if (cdda_model && cdda_model->cdio())
        device_widget->setDeviceInfo(cdda_model->cdio()->getVendor(),
                                     cdda_model->cdio()->getModel(),
                                     cdda_model->cdio()->getRevision(),
                                     cdda_model->cdio()->getDriveCapabilities().contains(READ_MCN),
                                     cdda_model->cdio()->getDriveCapabilities().contains(READ_ISRC),
                                     cdda_model->cdio()->getDriveCapabilities().contains(C2_ERRS));

    KPageWidgetItem *profilePage = dialog->addPage(new profileWidget(profile_model), i18n("Profiles"));
    profilePage->setIcon(QIcon::fromTheme("document-multiple"));

    KPluginMetaData info(QStringLiteral("plasma/kcms/systemsettings_qwidgets/kcm_cddb"));
    KCModule *m = KCModuleLoader::loadModule(info);
    if (m) {
        m->load();
        auto *cfg = new KCDDB::Config();
        cfg->load();
        dialog->addPage(m, cfg, i18n("CDDB settings"), "text-xmcd");
    }

    KPageWidgetItem *remoteServerPage = dialog->addPage(new remoteServerSettingsWidget(), i18n("Remote Server"));
    remoteServerPage->setIcon(QIcon::fromTheme("network-server"));

    connect(dialog, SIGNAL(settingsChanged(const QString &)), this, SLOT(configuration_updated(const QString &)));

    dialog->exec();
}

void MainWindow::edit()
{
    cdda_header_widget->edit_data();
}

void MainWindow::new_audio_disc_detected()
{
    enable_layout(true);
    resizeColumns();
    if (Preferences::cddbLookupAuto()) {
        qDebug() << "Performing CDDB auto lookup";
        QTimer::singleShot(0, this, SLOT(cddb_lookup()));
    }

    update_layout();

    if (device_widget) {
        if (cdda_model && cdda_model->cdio()) {
            device_widget->setDeviceInfo(cdda_model->cdio()->getVendor(),
                                         cdda_model->cdio()->getModel(),
                                         cdda_model->cdio()->getRevision(),
                                         cdda_model->cdio()->getDriveCapabilities().contains(READ_MCN),
                                         cdda_model->cdio()->getDriveCapabilities().contains(READ_ISRC),
                                         cdda_model->cdio()->getDriveCapabilities().contains(C2_ERRS));
        } else {
            device_widget->clearDeviceInfo();
        }
    }
}

void MainWindow::audio_disc_removed()
{
    enable_layout(false);

    update_layout();

    if (device_widget)
        device_widget->clearDeviceInfo();
}

void MainWindow::cddb_lookup_start()
{
}

void MainWindow::cddb_lookup_done(const bool successful)
{
    if (!successful) {
        ErrorDialog::show(this,
                          i18n("CDDB lookup failed, with the following error:\n%1", cdda_model->lastError().message()),
                          cdda_model->lastError().details(),
                          i18n("CDD Lookup Failure"));
    }
    update_layout();
    disable_cddb_submit();
    // if (Preferences::coverLookupAuto())
    //     cdda_header_widget->fetchCover();
}

void MainWindow::update_layout()
{
    if (!cdda_model->isVarious()) {
        cdda_tree_view->hideColumn(CDDA_MODEL_COLUMN_ARTIST_INDEX);
    } else {
        cdda_tree_view->showColumn(CDDA_MODEL_COLUMN_ARTIST_INDEX);
    }
    resizeColumns();
    actionCollection()->action("selectall")->setEnabled(cdda_model->selectedTracks().count() < cdda_model->numOfAudioTracks());
    actionCollection()->action("selectnone")->setEnabled(cdda_model->selectedTracks().count() > 0);
}

void MainWindow::enable_layout(bool enabled)
{
    layout_enabled = enabled;
    cdda_tree_view->setEnabled(enabled);
    cdda_header_dock->setEnabled(enabled);
    cdda_header_widget->setEnabled(enabled);
    actionCollection()->action("profile_label")->setEnabled((profile_model->rowCount() > 0) && (enabled));
    profile_combobox->setEnabled((profile_model->rowCount() > 0) && (enabled));
    actionCollection()->action("profile")->setEnabled((profile_model->rowCount() > 0) && (enabled));
    actionCollection()->action("cddbfetch")->setEnabled(enabled);
    if (cdda_model->isModified())
        actionCollection()->action("cddbsubmit")->setEnabled(enabled);
    else
        actionCollection()->action("cddbsubmit")->setEnabled(false);
    actionCollection()->action("edit")->setEnabled(enabled);
    actionCollection()->action("eject")->setEnabled(enabled);
    actionCollection()->action("rip")->setEnabled(enabled);
    actionCollection()->action("splittitles")->setEnabled(enabled);
    actionCollection()->action("swapartistsandtitles")->setEnabled(enabled);
    actionCollection()->action("capitalize")->setEnabled(enabled);
    actionCollection()->action("autofillartists")->setEnabled(enabled);
    actionCollection()->action("selectall")->setEnabled(enabled);
    actionCollection()->action("selectnone")->setEnabled(enabled);
    actionCollection()->action("invertselection")->setEnabled(enabled);
}

void MainWindow::enable_cddb_submit(bool enabled)
{
    actionCollection()->action("cddbsubmit")->setEnabled(enabled);
}

void MainWindow::disable_cddb_submit()
{
    actionCollection()->action("cddbsubmit")->setEnabled(false);
}

void MainWindow::configuration_updated(const QString &dialog_name)
{
    Q_UNUSED(dialog_name);
    Preferences::self()->save();
}

void MainWindow::current_profile_updated_from_ui(int row)
{
    if (row >= 0) {
        profile_model->blockSignals(true);
        profile_model->setRowAsCurrentProfileIndex(row);
        profile_model->blockSignals(false);
    }
}

void MainWindow::update_profile_action(int index)
{
    if (index == -1) {
        if (layout_enabled) {
            actionCollection()->action("profile_label")->setEnabled(false);
            actionCollection()->action("profile")->setEnabled(false);
        }
    } else {
        if (layout_enabled) {
            actionCollection()->action("profile_label")->setEnabled(true);
            actionCollection()->action("profile")->setEnabled(true);
        }
        profile_combobox->setCurrentIndex(profile_model->getRowByIndex(index));
    }
}

void MainWindow::update_profile_action()
{
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

void MainWindow::split_titles()
{
    bool ok;
    QString divider = QInputDialog::getText(this,
                                            i18n("Split titles"),
                                            i18n("Please set a divider string. Be aware of empty spaces.\n\nDivider:"),
                                            QLineEdit::Normal,
                                            " - ",
                                            &ok);
    if (ok && !divider.isEmpty()) {
        cdda_model->splitTitleOfTracks(divider);
    }
}

void MainWindow::swap_artists_and_titles()
{
    if (KMessageBox::warningTwoActions(this,
                                       i18n("Do you really want to swap all artists and titles?"),
                                       i18n("Swap artists and titles"),
                                       KStandardGuiItem::ok(),
                                       KStandardGuiItem::cancel(),
                                       "no_swap_artists_and_titles_warn")
        == KMessageBox::SecondaryAction)
        return;

    cdda_model->swapArtistAndTitle();
    cdda_model->swapArtistAndTitleOfTracks();
}

void MainWindow::capitalize()
{
    if (KMessageBox::warningTwoActions(this,
                                       i18n("Do you really want to capitalize all artists and titles?"),
                                       i18n("Capitalize artists and titles"),
                                       KStandardGuiItem::ok(),
                                       KStandardGuiItem::cancel(),
                                       "no_capitalize_warn")
        == KMessageBox::SecondaryAction)
        return;

    cdda_model->capitalizeHeader();
    cdda_model->capitalizeTracks();
}

void MainWindow::auto_fill_artists()
{
    if (KMessageBox::warningTwoActions(this,
                                       i18n("Do you really want to autofill track artists?"),
                                       i18n("Autofill artists"),
                                       KStandardGuiItem::ok(),
                                       KStandardGuiItem::cancel(),
                                       "no_autofill_warn")
        == KMessageBox::SecondaryAction)
        return;

    cdda_model->setTitleArtistsFromHeader();
}

void MainWindow::toggle(const QModelIndex &idx)
{
    if (idx.isValid() && (idx.column() == CDDA_MODEL_COLUMN_RIP_INDEX)) {
        cdda_model->toggle(idx.row());
        cdda_tree_view->update(idx);
    }
}

void MainWindow::resizeColumns()
{
    for (int i = 0; i < CDDA_MODEL_COLUMN_COUNT; ++i)
        cdda_tree_view->resizeColumnToContents(i);
}

void MainWindow::setup_actions()
{
    auto *ejectAction = new QAction(this);
    ejectAction->setText(i18n("Eject"));
    ejectAction->setIcon(QIcon::fromTheme("media-eject"));
    actionCollection()->addAction("eject", ejectAction);
    actionCollection()->setDefaultShortcut(ejectAction, Qt::CTRL + Qt::Key_E);
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

    auto *plabelAction = new QWidgetAction(this);
    plabelAction->setText(i18n("&Profile:"));
    plabelAction->setDefaultWidget(profile_label);
    profile_label->setBuddy(profile_combobox);
    actionCollection()->addAction("profile_label", plabelAction);

    auto *profileAction = new QWidgetAction(this);
    profileAction->setText(i18n("Profile"));
    profileAction->setDefaultWidget(profile_combobox);
    actionCollection()->addAction("profile", profileAction);
    actionCollection()->setDefaultShortcut(profileAction, Qt::Key_F6);
    actionCollection()->setShortcutsConfigurable(profileAction, false);
    update_profile_action();

    auto *cddbLookupAction = new QAction(this);
    cddbLookupAction->setText(i18n("Fetch"));
    cddbLookupAction->setIcon(QIcon::fromTheme("view-list-text"));
    actionCollection()->addAction("cddbfetch", cddbLookupAction);
    actionCollection()->setDefaultShortcut(cddbLookupAction, Qt::CTRL + Qt::Key_F);
    connect(cddbLookupAction, SIGNAL(triggered(bool)), this, SLOT(cddb_lookup()));

    auto *cddbSubmitAction = new QAction(this);
    cddbSubmitAction->setText(i18n("Submit"));
    actionCollection()->addAction("cddbsubmit", cddbSubmitAction);
    actionCollection()->setDefaultShortcut(cddbSubmitAction, Qt::CTRL + Qt::Key_S);
    connect(cddbSubmitAction, SIGNAL(triggered(bool)), this, SLOT(cddb_submit()));

    auto *editAction = new QAction(this);
    editAction->setText(i18n("Edit"));
    editAction->setIcon(QIcon::fromTheme("document-edit"));
    actionCollection()->addAction("edit", editAction);
    actionCollection()->setDefaultShortcut(editAction, Qt::CTRL + Qt::Key_D);
    connect(editAction, SIGNAL(triggered(bool)), this, SLOT(edit()));

    auto *extractAction = new QAction(this);
    extractAction->setText(i18n("Rip..."));
    extractAction->setIcon(QIcon::fromTheme("media-optical-audio"));
    actionCollection()->addAction("rip", extractAction);
    actionCollection()->setDefaultShortcut(extractAction, Qt::CTRL + Qt::Key_X);
    connect(extractAction, SIGNAL(triggered(bool)), this, SLOT(rip()));

    actionCollection()->addAction("preferences", KStandardAction::preferences(this, SLOT(configure()), this));

    auto *splitTitlesAction = new QAction(this);
    splitTitlesAction->setText(i18n("Split Titles..."));
    actionCollection()->addAction("splittitles", splitTitlesAction);
    connect(splitTitlesAction, SIGNAL(triggered(bool)), this, SLOT(split_titles()));

    auto *swapArtistsAndTitlesAction = new QAction(this);
    swapArtistsAndTitlesAction->setText(i18n("Swap Artists And Titles"));
    actionCollection()->addAction("swapartistsandtitles", swapArtistsAndTitlesAction);
    connect(swapArtistsAndTitlesAction, SIGNAL(triggered(bool)), this, SLOT(swap_artists_and_titles()));

    auto *capitalizeAction = new QAction(this);
    capitalizeAction->setText(i18n("Capitalize"));
    actionCollection()->addAction("capitalize", capitalizeAction);
    connect(capitalizeAction, SIGNAL(triggered(bool)), this, SLOT(capitalize()));

    auto *autoFillArtistsAction = new QAction(this);
    autoFillArtistsAction->setText(i18n("Auto Fill Artists"));
    actionCollection()->addAction("autofillartists", autoFillArtistsAction);
    connect(autoFillArtistsAction, SIGNAL(triggered(bool)), this, SLOT(auto_fill_artists()));

    auto *selectAllAction = new QAction(this);
    selectAllAction->setText(i18n("Select All Tracks"));
    actionCollection()->addAction("selectall", selectAllAction);
    connect(selectAllAction, SIGNAL(triggered(bool)), this, SLOT(select_all()));

    auto *selectNoneAction = new QAction(this);
    selectNoneAction->setText(i18n("Deselect All Tracks"));
    actionCollection()->addAction("selectnone", selectNoneAction);
    connect(selectNoneAction, SIGNAL(triggered(bool)), this, SLOT(select_none()));

    auto *invertSelectionAction = new QAction(this);
    invertSelectionAction->setText(i18n("Invert Selection"));
    actionCollection()->addAction("invertselection", invertSelectionAction);
    connect(invertSelectionAction, SIGNAL(triggered(bool)), this, SLOT(invert_selection()));

    KStandardAction::quit(qApp, SLOT(quit()), actionCollection());
}

void MainWindow::setup_layout()
{
    cdda_tree_view = new CDDATreeView(this);
    cdda_tree_view->setModel(cdda_model);
    cdda_tree_view->setAlternatingRowColors(true);
    cdda_tree_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    cdda_tree_view->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);
    cdda_tree_view->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    cdda_tree_view->setIndentation(0);
    cdda_tree_view->setAllColumnsShowFocus(true);
    cdda_tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(cdda_tree_view, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(cdda_context_menu(const QPoint &)));
    connect(cdda_tree_view, SIGNAL(clicked(const QModelIndex &)), SLOT(toggle(const QModelIndex &)));
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

    statusBar()->hide();
    statusBar()->setMaximumHeight(0);
}

void MainWindow::select_all()
{
    cdda_model->selectAll();
}

void MainWindow::select_none()
{
    cdda_model->selectNone();
}

void MainWindow::invert_selection()
{
    cdda_model->invertSelection();
}

void MainWindow::cdda_context_menu(const QPoint &pos)
{
    Q_UNUSED(pos);
    QMenu menu(this);
    menu.addAction(actionCollection()->action("selectall"));
    menu.addAction(actionCollection()->action("selectnone"));
    menu.addSeparator();
    menu.addAction(actionCollection()->action("invertselection"));
    menu.exec(QCursor::pos());
}

void MainWindow::selection_changed(const int num_selected)
{
    actionCollection()->action("rip")->setEnabled(num_selected > 0);
    actionCollection()->action("selectall")->setEnabled(num_selected < cdda_model->numOfAudioTracks());
    actionCollection()->action("selectnone")->setEnabled(num_selected > 0);
}
