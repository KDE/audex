/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mainwindow.h"

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
    bool updated = firstStart();

    QObject::connect(&manager, &Audex::Device::Manager::audioDiscDetected, [this](const QString &driveUDI, const QString &discUDI, const Audex::CDDA &cdda) {
        Q_UNUSED(driveUDI);
        if (!manager.currentDiscAvailable()) {
            enable_layout(true);
            cdda_model.update(cdda);
            manager.setCurrentDisc(discUDI);
        }
        if (Preferences::cddbLookupAuto())
            manager.lookupCDDB(discUDI);
        disc_combobox->blockSignals(true);
        disc_combobox->addItem(QIcon::fromTheme("media-optical-audio"), manager.discPrettyName(discUDI), discUDI);
        disc_combobox->blockSignals(false);
        update_layout();
        update_disc_action();
    });

    QObject::connect(&manager, &Audex::Device::Manager::audioDiscRemoved, [this](const QString &discUDI) {
        if (manager.currentDisc() == discUDI) {
            cdda_model.clear();
            manager.clearCurrentDisc();
        }
        disc_combobox->removeItem(disc_combobox->findData(discUDI));
        if (!manager.currentDiscAvailable()) {
            enable_layout(false);
        }
        update_disc_action();
    });

    QObject::connect(&manager,
                     &Audex::Device::Manager::CDDBLookupDone,
                     [this](const QString &discUDI, const bool successful, const Audex::Metadata::Metadata &metadata) {
                         if (successful && manager.currentDisc() == discUDI) {
                             Audex::CDDA cdda = manager.cdda(discUDI);
                             // Keep the cover
                             const QImage cover = cdda.metadata().cover();
                             cdda.setMetadata(metadata);
                             cdda.metadata().setCover(cover);
                             cdda_model.update(cdda);
                             cdda_header_widget->setCDDA(cdda);
                             update_layout();
                         }
                     });
    QObject::connect(&manager,
                     &Audex::Device::Manager::CDTextReadDone,
                     [this](const QString &discUDI, const bool successful, const Audex::Metadata::Metadata &metadata) {
                         if (successful && manager.currentDisc() == discUDI) {
                             Audex::CDDA cdda = manager.cdda(discUDI);
                             const QImage cover = cdda.metadata().cover();
                             // Keep the cover
                             cdda.setMetadata(metadata);
                             cdda.metadata().setCover(cover);
                             cdda_model.update(cdda);
                             cdda_header_widget->setCDDA(cdda);
                             update_layout();
                         }
                     });

    QObject::connect(&cdda_model, &Audex::CDDAModel::metadataChanged, &manager, &Audex::Device::Manager::setMetadataInCurrentDisc);

    QObject::connect(&cdda_model, &Audex::CDDAModel::selectionChanged, [this](const int num_selected) {
        actionCollection()->action("rip")->setEnabled(num_selected > 0);
        actionCollection()->action("selectall")->setEnabled(num_selected < manager.currentCDDA().toc().audioTrackCount());
        actionCollection()->action("selectnone")->setEnabled(num_selected > 0);
    });

    QObject::connect(&profile_model, &Audex::ProfileModel::profilesRemovedOrInserted, this, &MainWindow::update_profile_action);
    QObject::connect(&profile_model, &Audex::ProfileModel::currentProfileIndexChanged, this, &MainWindow::update_profile_action_by_index);

    setup_actions();
    setup_layout();
    setupGUI();

    enable_layout(false);

    if (updated) {
        update();
        resize(1024, 768);
    }

    QTimer::singleShot(0, &manager, &Audex::Device::Manager::scanBus);
}

bool MainWindow::firstStart()
{
    if (Preferences::firstStart()) {
        profile_model.autoCreate();
        Preferences::setFirstStart(false);
        Preferences::self()->save();
        return true;
    }

    return false;
}

void MainWindow::eject()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << disc_combobox->currentIndex();
    manager.ejectDisc(manager.currentDisc());
}

void MainWindow::cddb_lookup()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << disc_combobox->currentIndex();
    manager.lookupCDDB(manager.currentDisc());
}

void MainWindow::cdtext_read()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << disc_combobox->currentIndex();
    manager.readCDText(manager.currentDisc());
}

void MainWindow::rip()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << disc_combobox->currentIndex() << cdda_model.selectedTracks();

    if (manager.currentCDDA().metadata().isEmpty()) {
        if (KMessageBox::warningTwoActions(this,
                                           i18n("No disc information set. Do you really want to continue?"),
                                           i18n("Disc information not found"),
                                           KStandardGuiItem::cont(),
                                           KStandardGuiItem::cancel(),
                                           "no_disc_info_warn")
            == KMessageBox::SecondaryAction)
            return;
    }

    if ((profile_model.data(profile_model.index(profile_model.currentProfileRow(), Audex::PROFILE_MODEL_COLUMN_SF_INDEX)).toBool())
        && (cdda_model.selectedTracks().count() < manager.currentCDDA().toc().audioTrackCount())) {
        if (KMessageBox::warningTwoActions(this,
                                           i18n("Single file rip selected but not all audio tracks to rip selected. Do you really want to continue?"),
                                           i18n("Not all audio tracks selected for single file rip"),
                                           KStandardGuiItem::cont(),
                                           KStandardGuiItem::cancel(),
                                           "singlefile_selection_warn")
            == KMessageBox::SecondaryAction)
            return;
    }

    if ((profile_model.isSelectedEncoderWithEmbedCover()
         || (profile_model.data(profile_model.index(profile_model.currentProfileRow(), Audex::PROFILE_MODEL_COLUMN_SC_INDEX)).toBool()))
        && manager.currentCDDA().metadata().cover().isNull()) {
        if (KMessageBox::warningTwoActions(this,
                                           i18n("No cover was set. Do you really want to continue?"),
                                           i18n("Cover is empty"),
                                           KStandardGuiItem::cont(),
                                           KStandardGuiItem::cancel(),
                                           "empty_cover_warn")
            == KMessageBox::SecondaryAction)
            return;
    }

    auto *dialog = new Audex::ExtractingProgressDialog(&profile_model,
                                                       manager.currentCDDA(),
                                                       cdda_model.selectedTracks(),
                                                       manager.blockDeviceFromDisc(manager.currentDisc()),
                                                       this);

    dialog->setWindowModality(Qt::ApplicationModal);

    dialog->exec();

    delete dialog;
}

void MainWindow::configure()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    if (KConfigDialog::showDialog("settings"))
        return;

    KConfigDialog *dialog = new KConfigDialog(this, "settings", Preferences::self());

    KPageWidgetItem *generalPage = dialog->addPage(new Audex::generalSettingsWidget(), i18n("General settings"));
    generalPage->setIcon(QIcon(QApplication::windowIcon()));

    KPageWidgetItem *devicePage = dialog->addPage(new Audex::deviceWidget(), i18n("Device settings"));
    devicePage->setIcon(QIcon::fromTheme("drive-optical"));

    KPageWidgetItem *profilePage = dialog->addPage(new Audex::profileWidget(&profile_model), i18n("Profiles"));
    profilePage->setIcon(QIcon::fromTheme("document-multiple"));

    KPluginMetaData info(QStringLiteral("plasma/kcms/systemsettings_qwidgets/kcm_cddb"));
    KCModule *m = KCModuleLoader::loadModule(info);
    if (m) {
        m->load();
        auto *cfg = new KCDDB::Config();
        cfg->load();
        dialog->addPage(m->widget(), cfg, i18n("CDDB settings"), QStringLiteral("text-xmcd"));
    }

    KPageWidgetItem *remoteServerPage = dialog->addPage(new Audex::remoteServerSettingsWidget(), i18n("Remote Server"));
    remoteServerPage->setIcon(QIcon::fromTheme("network-server"));

    connect(dialog, SIGNAL(settingsChanged(const QString &)), this, SLOT(configuration_updated(const QString &)));

    dialog->exec();
}

void MainWindow::edit()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;
    cdda_header_widget->editData();
}

void MainWindow::update_layout()
{
    if (cdda_model.isVarious()) {
        cdda_tree_view->showColumn(Audex::CDDAModel::COLUMN_ARTIST_INDEX);
    } else {
        cdda_tree_view->hideColumn(Audex::CDDAModel::COLUMN_ARTIST_INDEX);
    }
    resizeColumns();
}

void MainWindow::enable_layout(bool enabled)
{
    layout_enabled = enabled;
    cdda_tree_view->setEnabled(enabled);
    cdda_header_dock->setEnabled(enabled);
    cdda_header_widget->setEnabled(enabled);
    actionCollection()->action("disc")->setEnabled(manager.currentDiscAvailable() && enabled);
    actionCollection()->action("profile_label")->setEnabled((profile_model.rowCount() > 0) && enabled);
    profile_combobox->setEnabled((profile_model.rowCount() > 0) && enabled);
    actionCollection()->action("profile")->setEnabled((profile_model.rowCount() > 0) && enabled);
    actionCollection()->action("cddbfetch")->setEnabled(enabled);
    actionCollection()->action("cdtextread")->setEnabled(enabled);
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

void MainWindow::configuration_updated(const QString &dialog_name)
{
    Q_UNUSED(dialog_name);
    Preferences::self()->save();
}

void MainWindow::update_disc_action()
{
    actionCollection()->action("disc")->setEnabled(manager.currentDiscAvailable());
}

void MainWindow::current_profile_updated_from_ui(int row)
{
    if (row >= 0) {
        profile_model.blockSignals(true);
        profile_model.setRowAsCurrentProfileIndex(row);
        profile_model.blockSignals(false);
    }
}

void MainWindow::update_profile_action_by_index(int index)
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
        profile_combobox->setCurrentIndex(profile_model.getRowByIndex(index));
    }
}

void MainWindow::update_profile_action()
{
    // If the profile model emits 'reset' the profile combo clears its current settings.
    // Therefore, we need to try and reset these...
    if (profile_combobox->currentText().isEmpty()) {
        profile_combobox->setCurrentIndex(profile_model.currentProfileRow());
    }

    if (layout_enabled) {
        actionCollection()->action("profile_label")->setEnabled(profile_model.rowCount() > 0);
        actionCollection()->action("profile")->setEnabled(profile_model.rowCount() > 0);
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
        cdda_model.splitTitleOfTracks(divider);
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

    cdda_model.swapArtistAndAlbum();
    cdda_model.swapArtistAndTitleOfTracks();
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

    cdda_model.capitalizeHeader();
    cdda_model.capitalizeTracks();
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

    cdda_model.setTitleArtistsFromHeader();
}

void MainWindow::resizeColumns()
{
    for (int i = 0; i < Audex::CDDAModel::COLUMN_COUNT; ++i)
        cdda_tree_view->resizeColumnToContents(i);
}

void MainWindow::setup_actions()
{
    disc_combobox = new KComboBox(this);
    disc_combobox->setMinimumWidth(80);
    disc_combobox->setMaximumWidth(220);
    disc_combobox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    disc_combobox->resize(QSize(220, disc_combobox->height()));
    QObject::connect(disc_combobox, &QComboBox::currentIndexChanged, [this](int index) {
        QString discUDI = disc_combobox->itemData(index).toString();
        if (manager.currentDisc() != discUDI) {
            manager.setCurrentDisc(discUDI);
            Audex::CDDA cdda = manager.currentCDDA();
            cdda_model.update(cdda);
            cdda_header_widget->setCDDA(cdda);
            update_layout();
        }
    });

    disc_label = new QLabel(this);
    disc_label->setText(i18n("Disc:"));
    auto *dlabelAction = new QWidgetAction(this);
    dlabelAction->setText(i18n("&Disc:"));
    dlabelAction->setDefaultWidget(disc_label);
    disc_label->setBuddy(disc_combobox);
    actionCollection()->addAction("disc_label", dlabelAction);

    auto *discAction = new QWidgetAction(this);
    discAction->setText(i18n("Disc"));
    discAction->setDefaultWidget(disc_combobox);
    actionCollection()->addAction("disc", discAction);
    actionCollection()->setDefaultShortcut(discAction, Qt::Key_F8);
    actionCollection()->setShortcutsConfigurable(discAction, false);
    update_disc_action();

    auto *ejectAction = new QAction(this);
    ejectAction->setText(i18n("Eject"));
    ejectAction->setIcon(QIcon::fromTheme("media-eject"));
    actionCollection()->addAction("eject", ejectAction);
    actionCollection()->setDefaultShortcut(ejectAction, Qt::CTRL | Qt::Key_E);
    QObject::connect(ejectAction, &QAction::triggered, this, &MainWindow::eject);

    profile_combobox = new KComboBox(this);
    profile_combobox->setModel(&profile_model);
    profile_combobox->setModelColumn(1);
    profile_combobox->setMinimumWidth(80);
    profile_combobox->setMaximumWidth(220);
    profile_combobox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    profile_combobox->resize(QSize(220, profile_combobox->height()));
    profile_combobox->setCurrentIndex(profile_model.currentProfileRow());
    QObject::connect(profile_combobox, &KComboBox::currentIndexChanged, this, &MainWindow::current_profile_updated_from_ui);

    profile_label = new QLabel(this);
    profile_label->setText(i18n("Profile:"));
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
    actionCollection()->setDefaultShortcut(cddbLookupAction, Qt::CTRL | Qt::Key_F);
    QObject::connect(cddbLookupAction, &QAction::triggered, this, &MainWindow::cddb_lookup);

    auto *cdTextReadAction = new QAction(this);
    cdTextReadAction->setText(i18n("Read CD-Text"));
    cdTextReadAction->setIcon(QIcon::fromTheme("view-list-text"));
    actionCollection()->addAction("cdtextread", cdTextReadAction);
    actionCollection()->setDefaultShortcut(cdTextReadAction, Qt::CTRL | Qt::Key_T);
    QObject::connect(cdTextReadAction, &QAction::triggered, this, &MainWindow::cdtext_read);

    auto *editAction = new QAction(this);
    editAction->setText(i18n("Edit"));
    editAction->setIcon(QIcon::fromTheme("document-edit"));
    actionCollection()->addAction("edit", editAction);
    actionCollection()->setDefaultShortcut(editAction, Qt::CTRL | Qt::Key_D);
    QObject::connect(editAction, &QAction::triggered, this, &MainWindow::edit);

    auto *ripAction = new QAction(this);
    ripAction->setText(i18n("Rip..."));
    ripAction->setIcon(QIcon::fromTheme("media-optical-audio"));
    actionCollection()->addAction("rip", ripAction);
    actionCollection()->setDefaultShortcut(ripAction, Qt::CTRL | Qt::Key_X);
    QObject::connect(ripAction, &QAction::triggered, this, &MainWindow::rip);

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
    QObject::connect(selectAllAction, &QAction::triggered, [this]() {
        cdda_model.selectAll();
    });

    auto *selectNoneAction = new QAction(this);
    selectNoneAction->setText(i18n("Deselect All Tracks"));
    actionCollection()->addAction("selectnone", selectNoneAction);
    QObject::connect(selectNoneAction, &QAction::triggered, [this]() {
        cdda_model.selectNone();
    });

    auto *invertSelectionAction = new QAction(this);
    invertSelectionAction->setText(i18n("Invert Selection"));
    actionCollection()->addAction("invertselection", invertSelectionAction);
    QObject::connect(invertSelectionAction, &QAction::triggered, [this]() {
        cdda_model.invertSelection();
    });

    KStandardAction::quit(qApp, SLOT(quit()), actionCollection());
}

void MainWindow::setup_layout()
{
    cdda_tree_view = new CDDATreeView(this);
    cdda_tree_view->setModel(&cdda_model);
    cdda_tree_view->setAlternatingRowColors(true);
    cdda_tree_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    cdda_tree_view->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);
    cdda_tree_view->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    cdda_tree_view->setIndentation(0);
    cdda_tree_view->setAllColumnsShowFocus(true);
    cdda_tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(cdda_tree_view, &QTreeView::customContextMenuRequested, [this](const QPoint &) {
        QMenu menu(cdda_tree_view);
        menu.addAction(actionCollection()->action("selectall"));
        menu.addAction(actionCollection()->action("selectnone"));
        menu.addSeparator();
        menu.addAction(actionCollection()->action("invertselection"));
        menu.exec(QCursor::pos());
    });

    cdda_header_dock = new QDockWidget(this);
    cdda_header_dock->setObjectName("cdda_header_dock");
    cdda_header_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    cdda_header_dock->setAllowedAreas(Qt::AllDockWidgetAreas);

    setCentralWidget(cdda_tree_view);
    cdda_header_widget = new Audex::CDDAHeaderWidget(cdda_header_dock);
    QObject::connect(&cdda_model, &Audex::CDDAModel::metadataChanged, cdda_header_widget, &Audex::CDDAHeaderWidget::setMetadata);
    QObject::connect(cdda_header_widget, &Audex::CDDAHeaderWidget::metadataChanged, [this](const Audex::Metadata::Metadata &metadata) {
        Audex::CDDA cdda = manager.currentCDDA();
        cdda.setMetadata(metadata);
        cdda_model.update(cdda);
        cdda_header_widget->setCDDA(cdda);
        update_layout();
    });
    QObject::connect(cdda_header_widget, &Audex::CDDAHeaderWidget::metadataChanged, &manager, &Audex::Device::Manager::setMetadataInCurrentDisc);
    cdda_header_dock->setWidget(cdda_header_widget);
    addDockWidget(Qt::LeftDockWidgetArea, cdda_header_dock);

    statusBar()->hide();
    statusBar()->setMaximumHeight(0);
}
