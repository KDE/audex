/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mainwindow.h"

#include <algorithm>

#include "core/cdg.h"
#include "encoding/registry.h"
#include "models/cdinfomodel.h"
#include "models/profilemodel.h"
#include "online/gnudbprovider.h"
#include "online/musicbrainzprovider.h"
#include "utils/devicesettings.h"
#include "utils/disccontroller.h"
#include "utils/encoderassistant.h"

#include "preferences.h"

#include "widgets/cddaheaderwidget.h"
#include "widgets/devicewidget.h"
#include "widgets/discplaceholder.h"
#include "widgets/generalsettingswidget.h"
#include "widgets/profilewidget.h"

#include "dialogs/coverchooserdialog.h"
#include "dialogs/errordialog.h"
#include "dialogs/extractingprogressdialog.h"
#include "dialogs/metadatacandidatedialog.h"
#include "dialogs/settingsdialog.h"

#include <KActionCollection>
#include <KComboBox>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageWidget>
#include <KStandardAction>
#include <KStandardGuiItem>

#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QDockWidget>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QtConcurrent>

using namespace Qt::StringLiterals;

namespace
{

class CDDATreeView : public QTreeView
{
public:
    explicit CDDATreeView(QWidget *parent = nullptr)
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

QString preferredProviderId()
{
    switch (Preferences::metadataProvider()) {
    case Preferences::EnumMetadataProvider::CDText:
        return u"cdtext"_s;
    case Preferences::EnumMetadataProvider::Gnudb:
        return u"gnudb"_s;
    default:
        return u"musicbrainz"_s;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// construction
// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    // encoder registry with the encoder plugins; the native backends make
    // the matching profiles available even without the external binaries
    m_encoders = std::make_shared<Audex::Encoding::EncoderRegistry>();
    m_encoders->loadPlugins();
    // make plugin discovery problems visible on the terminal: without the
    // encoder plugins only WAVE and Custom (external command) are available
    qInfo().noquote() << "Encoder plugin search directories:" << Audex::Encoding::EncoderRegistry::defaultPluginDirectories().join(u", "_s);
    for (const QString &line : m_encoders->diagnostics())
        qInfo().noquote() << "Encoder plugin:" << line;
    // profiles that need a missing plugin are disabled with a hint in the
    // main window (see updateProfileMessage())
    if (!m_encoders->factory(u"flac"_s) && !m_encoders->factory(u"mp3"_s) && !m_encoders->factory(u"opus"_s))
        qWarning() << "No encoder plugins found - only WAVE and Custom (external command) are available. "
                      "Re-run cmake with the codec development packages installed, or set AUDEX_ENCODER_PLUGIN_PATH.";
    if (auto *f = m_encoders->factory(u"mp3"_s))
        EncoderAssistant::setNativeBackend(EncoderAssistant::LAME, f->version());
    if (auto *f = m_encoders->factory(u"flac"_s))
        EncoderAssistant::setNativeBackend(EncoderAssistant::FLAC, f->version());
    if (auto *f = m_encoders->factory(u"opus"_s))
        EncoderAssistant::setNativeBackend(EncoderAssistant::OPUSENC, f->version());

    // after the plugins: loading migrates profiles, which depends on them
    m_profileModel = new ProfileModel(this);
    if (m_profileModel->lastError().isValid()) {
        ErrorDialog::show(this, m_profileModel->lastError().message(), m_profileModel->lastError().details());
        return;
    }

    const bool isFirstStart = firstStart();
    m_profileModel->ensureAvailableCurrentProfile();

    m_cddaModel = new Audex::CDInfoModel(this);

    m_discController = new DiscController(this);
    connect(m_discController, &DiscController::discDetected, this, &MainWindow::disc_detected);
    connect(m_discController, &DiscController::discRemoved, this, &MainWindow::disc_removed);
    connect(m_discController, &DiscController::drivesChanged, this, &MainWindow::drives_updated);
    connect(m_discController, &DiscController::currentDriveChanged, this, &MainWindow::current_drive_updated);

    m_network = new QNetworkAccessManager(this);

    m_lookup = new Audex::MetadataLookup(this);
    m_cdtextProvider = new Audex::PrecomputedProvider(u"cdtext"_s, i18n("CD-Text"), this);
    m_lookup->addProvider(m_cdtextProvider);
    m_lookup->addProvider(new Audex::MusicBrainzProvider(m_network, this));
    m_gnudbProvider = new Audex::GnudbProvider(m_network, this);
    m_lookup->addProvider(m_gnudbProvider);
    connect(m_lookup, &Audex::MetadataLookup::finished, this, &MainWindow::lookup_finished);
    connect(m_lookup, &Audex::MetadataLookup::providerFailed, this, &MainWindow::lookup_provider_failed);

    m_coverFetcher = new Audex::CoverArtFetcher(m_network, this);
    connect(m_coverFetcher, &Audex::CoverArtFetcher::finished, this, &MainWindow::cover_fetch_finished);

    connect(m_cddaModel, &Audex::CDInfoModel::metadataChanged, this, &MainWindow::update_layout);

    connect(m_profileModel, &ProfileModel::profilesRemovedOrInserted, this, [this]() {
        update_profile_action();
        profileChanged();
        updateProfileMessage();
    });

    connect(m_profileModel, &ProfileModel::currentProfileIndexChanged, this, [this](int index) {
        update_profile_action(index);
        profileChanged();
    });

    // a profile's encoder or output type may have been edited
    connect(m_profileModel, &QAbstractItemModel::dataChanged, this, [this]() {
        profileChanged();
        updateProfileMessage();
    });

    connect(&m_hdcdWatcher, &QFutureWatcherBase::finished, this, &MainWindow::hdcd_probe_finished);
    connect(&m_cdgWatcher, &QFutureWatcherBase::finished, this, &MainWindow::cdg_probe_finished);

    setup_actions();
    setup_layout();
    setupGUI();

    drives_updated(); // initial fill of the drive selector
    enable_layout(false);

    profileChanged();
    updateProfileMessage();

    if (isFirstStart)
        resize(650, 500);

    m_discController->rescan();

    m_valid = true;
}

MainWindow::~MainWindow()
{
    // the models are deleted before the base class destroys the widgets
    // using them (QPointer: safe even if the constructor returned early)
    delete m_profileModel.data();
    delete m_cddaModel.data();
}

bool MainWindow::isValid() const
{
    return m_valid;
}

bool MainWindow::firstStart()
{
    if (Preferences::firstStart()) {
        m_profileModel->autoCreate();
        Preferences::setFirstStart(false);
        Preferences::self()->save();
        return true;
    }

    return false;
}

void MainWindow::setup_actions()
{
    auto *ejectAction = new QAction(this);
    ejectAction->setText(i18n("Eject"));
    ejectAction->setIcon(QIcon::fromTheme("media-eject"));
    actionCollection()->addAction("eject", ejectAction);
    actionCollection()->setDefaultShortcut(ejectAction, Qt::CTRL | Qt::Key_E);
    connect(ejectAction, &QAction::triggered, this, &MainWindow::eject);

    // drive selector for the toolbar (between "Eject" and "Profile:")
    m_driveComboBox = new KComboBox(this);
    m_driveComboBox->setMinimumWidth(80);
    m_driveComboBox->setMaximumWidth(320);
    m_driveComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    m_driveComboBox->setToolTip(i18n("Select the drive to rip from"));
    connect(m_driveComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::current_drive_updated_from_ui);

    auto *driveAction = new QWidgetAction(this);
    driveAction->setText(i18n("Drive"));
    driveAction->setIcon(QIcon::fromTheme("drive-optical"));
    driveAction->setDefaultWidget(m_driveComboBox);
    actionCollection()->addAction("drive", driveAction);

    m_profileLabel = new QLabel(this);
    m_profileLabel->setText(i18n("Profile:"));
    m_profileComboBox = new KComboBox(this);
    m_profileFilter = new ProfileFilterModel(this);
    m_profileFilter->setSourceModel(m_profileModel);
    m_profileComboBox->setModel(m_profileFilter);
    m_profileComboBox->setModelColumn(1);
    m_profileComboBox->setMinimumWidth(80);
    m_profileComboBox->setMaximumWidth(220);
    m_profileComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    m_profileComboBox->setCurrentIndex(m_profileModel->currentProfileRow());
    connect(m_profileComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::current_profile_updated_from_ui);

    auto *plabelAction = new QWidgetAction(this);
    plabelAction->setText(i18n("&Profile:"));
    plabelAction->setDefaultWidget(m_profileLabel);
    m_profileLabel->setBuddy(m_profileComboBox);
    actionCollection()->addAction("profile_label", plabelAction);

    auto *profileAction = new QWidgetAction(this);
    profileAction->setText(i18n("Profile"));
    profileAction->setDefaultWidget(m_profileComboBox);
    actionCollection()->addAction("profile", profileAction);
    actionCollection()->setDefaultShortcut(profileAction, Qt::Key_F6);
    actionCollection()->setShortcutsConfigurable(profileAction, false);
    update_profile_action();

    // one action per metadata provider (shown in the menu and in the
    // drop-down of the fetch button)
    auto *cdtextAction = new QAction(this);
    cdtextAction->setText(i18n("CD-Text"));
    actionCollection()->addAction("cddbfetch_cdtext", cdtextAction);
    connect(cdtextAction, &QAction::triggered, this, &MainWindow::fetch_metadata_cdtext);

    auto *musicBrainzAction = new QAction(this);
    musicBrainzAction->setText(i18n("MusicBrainz"));
    actionCollection()->addAction("cddbfetch_musicbrainz", musicBrainzAction);
    connect(musicBrainzAction, &QAction::triggered, this, &MainWindow::fetch_metadata_musicbrainz);

    auto *gnudbAction = new QAction(this);
    gnudbAction->setText(i18n("gnudb (CDDB)"));
    actionCollection()->addAction("cddbfetch_gnudb", gnudbAction);
    connect(gnudbAction, &QAction::triggered, this, &MainWindow::fetch_metadata_gnudb);

    // fetch button with a drop-down for the metadata provider (toolbar)
    auto *fetchButton = new QToolButton(this);
    fetchButton->setPopupMode(QToolButton::MenuButtonPopup);

    auto *fetchAction = new QAction(this);
    fetchAction->setText(i18n("Fetch"));
    fetchAction->setIcon(QIcon::fromTheme("view-list-text"));
    connect(fetchAction, &QAction::triggered, this, &MainWindow::fetch_metadata);
    fetchButton->setDefaultAction(fetchAction);

    auto *fetchMenu = new QMenu(fetchButton);
    fetchMenu->addAction(cdtextAction);
    fetchMenu->addAction(musicBrainzAction);
    fetchMenu->addAction(gnudbAction);
    fetchButton->setMenu(fetchMenu);

    auto *fetchWidgetAction = new QWidgetAction(this);
    fetchWidgetAction->setText(i18n("Fetch"));
    fetchWidgetAction->setIcon(QIcon::fromTheme("view-list-text"));
    fetchWidgetAction->setDefaultWidget(fetchButton);
    actionCollection()->addAction("cddbfetch", fetchWidgetAction);
    actionCollection()->setDefaultShortcut(fetchAction, Qt::CTRL | Qt::Key_F);

    auto *editAction = new QAction(this);
    editAction->setText(i18n("Edit"));
    editAction->setIcon(QIcon::fromTheme("document-edit"));
    actionCollection()->addAction("edit", editAction);
    actionCollection()->setDefaultShortcut(editAction, Qt::CTRL | Qt::Key_D);
    connect(editAction, &QAction::triggered, this, &MainWindow::edit);

    auto *extractAction = new QAction(this);
    extractAction->setText(i18n("Rip..."));
    extractAction->setIcon(QIcon::fromTheme("media-optical-audio"));
    actionCollection()->addAction("rip", extractAction);
    actionCollection()->setDefaultShortcut(extractAction, Qt::CTRL | Qt::Key_X);
    connect(extractAction, &QAction::triggered, this, &MainWindow::rip);

    actionCollection()->addAction("preferences", KStandardAction::preferences(this, &MainWindow::configure, this));

    auto *splitTitlesAction = new QAction(this);
    splitTitlesAction->setText(i18n("Split Titles..."));
    actionCollection()->addAction("splittitles", splitTitlesAction);
    connect(splitTitlesAction, &QAction::triggered, this, &MainWindow::split_titles);

    auto *swapArtistsAndTitlesAction = new QAction(this);
    swapArtistsAndTitlesAction->setText(i18n("Swap Artists And Titles"));
    actionCollection()->addAction("swapartistsandtitles", swapArtistsAndTitlesAction);
    connect(swapArtistsAndTitlesAction, &QAction::triggered, this, &MainWindow::swap_artists_and_titles);

    auto *capitalizeAction = new QAction(this);
    capitalizeAction->setText(i18n("Capitalize"));
    actionCollection()->addAction("capitalize", capitalizeAction);
    connect(capitalizeAction, &QAction::triggered, this, &MainWindow::capitalize);

    auto *autoFillArtistsAction = new QAction(this);
    autoFillArtistsAction->setText(i18n("Auto Fill Artists"));
    actionCollection()->addAction("autofillartists", autoFillArtistsAction);
    connect(autoFillArtistsAction, &QAction::triggered, this, &MainWindow::auto_fill_artists);

    auto *selectAllAction = new QAction(this);
    selectAllAction->setText(i18n("Select All Tracks"));
    actionCollection()->addAction("selectall", selectAllAction);
    connect(selectAllAction, &QAction::triggered, this, &MainWindow::select_all);

    auto *selectNoneAction = new QAction(this);
    selectNoneAction->setText(i18n("Deselect All Tracks"));
    actionCollection()->addAction("selectnone", selectNoneAction);
    connect(selectNoneAction, &QAction::triggered, this, &MainWindow::select_none);

    auto *invertSelectionAction = new QAction(this);
    invertSelectionAction->setText(i18n("Invert Selection"));
    actionCollection()->addAction("invertselection", invertSelectionAction);
    connect(invertSelectionAction, &QAction::triggered, this, &MainWindow::invert_selection);

    KStandardAction::quit(qApp, &QCoreApplication::quit, actionCollection());
}

void MainWindow::setup_layout()
{
    m_cddaTreeView = new CDDATreeView(this);
    m_cddaTreeView->setModel(m_cddaModel);
    m_cddaTreeView->setAlternatingRowColors(true);
    m_cddaTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cddaTreeView->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);
    m_cddaTreeView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_cddaTreeView->setIndentation(0);
    m_cddaTreeView->setAllColumnsShowFocus(true);
    m_cddaTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_cddaTreeView, &QWidget::customContextMenuRequested, this, &MainWindow::cdda_context_menu);
    connect(m_cddaTreeView, &QAbstractItemView::clicked, this, &MainWindow::toggle);
    connect(m_cddaModel, &Audex::CDInfoModel::selectionChanged, this, &MainWindow::updateSelectionActionStates);

    m_cddaHeaderDock = new QDockWidget(this);
    m_cddaHeaderDock->setObjectName("cdda_header_dock");
    m_cddaHeaderDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_cddaHeaderDock->setAllowedAreas(Qt::AllDockWidgetAreas);

    m_profileMessage = new KMessageWidget(this);
    m_profileMessage->setMessageType(KMessageWidget::Warning);
    m_profileMessage->setPosition(KMessageWidget::Header);
    m_profileMessage->setWordWrap(true);
    m_profileMessage->setToolTip(i18n("Encoder plugins are searched in:\n%1\n\nThe environment variable AUDEX_ENCODER_PLUGIN_PATH adds further folders.",
                                      Audex::Encoding::EncoderRegistry::defaultPluginDirectories().join(u'\n')));
    m_profileMessage->hide();

    auto *central = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(m_profileMessage);
    m_placeholder = new DiscPlaceholder(this);
    connect(m_placeholder, &DiscPlaceholder::ejectClicked, this, &MainWindow::eject);
    connect(m_placeholder, &DiscPlaceholder::retryClicked, m_discController, &DiscController::retry);
    connect(m_placeholder, &DiscPlaceholder::detailsClicked, this, [this]() {
        ErrorDialog::show(this, m_discController->failureMessage(), m_discController->failureDetails());
    });
    m_trackStack = new QStackedWidget(this);
    m_trackStack->addWidget(m_cddaTreeView);
    m_trackStack->addWidget(m_placeholder);
    centralLayout->addWidget(m_trackStack);
    connect(m_discController, &DiscController::stateChanged, this, &MainWindow::update_placeholder);
    connect(m_discController, &DiscController::currentDriveChanged, this, &MainWindow::update_placeholder);
    update_placeholder();

    setCentralWidget(central);

    m_cddaHeaderWidget = new CDDAHeaderWidget(m_cddaModel, m_cddaHeaderDock);
    connect(m_cddaHeaderWidget, &CDDAHeaderWidget::headerDataChanged, this, &MainWindow::update_layout);
    connect(m_cddaHeaderWidget, &CDDAHeaderWidget::coverFetchRequested, this, [this]() {
        start_cover_fetch(m_coverCandidate);
    });
    m_cddaHeaderDock->setWidget(m_cddaHeaderWidget);
    addDockWidget(Qt::LeftDockWidgetArea, m_cddaHeaderDock);

    statusBar()->hide();
    statusBar()->setMaximumHeight(0);
}

// ---------------------------------------------------------------------------
// toolbar / menu actions
// ---------------------------------------------------------------------------

void MainWindow::eject()
{
    qDebug() << "eject requested";
    m_discController->eject();
}

void MainWindow::fetch_metadata()
{
    start_metadata_lookup(preferredProviderId(), false);
}

void MainWindow::fetch_metadata_cdtext()
{
    start_metadata_lookup(u"cdtext"_s, false);
}

void MainWindow::fetch_metadata_musicbrainz()
{
    start_metadata_lookup(u"musicbrainz"_s, false);
}

void MainWindow::fetch_metadata_gnudb()
{
    start_metadata_lookup(u"gnudb"_s, false);
}

void MainWindow::edit()
{
    m_cddaHeaderWidget->edit_data();
}

void MainWindow::rip()
{
    if (m_cddaModel->isEmpty()) {
        if (KMessageBox::warningTwoActions(this,
                                           i18n("No disc information set. Do you really want to continue?"),
                                           i18n("Disc information not found"),
                                           KStandardGuiItem::cont(),
                                           KStandardGuiItem::cancel(),
                                           "no_disc_info_warn")
            == KMessageBox::SecondaryAction)
            return;
    }

    const int profileRow = m_profileModel->currentProfileRow();
    if (!m_profileModel->isAvailable(profileRow)) {
        const QString reason = m_profileModel->unavailableReason(profileRow);
        KMessageBox::error(this, reason.isEmpty() ? i18n("No usable profile is selected.") : reason, i18n("Profile not available"));
        return;
    }

    if (m_profileModel->isImage(profileRow) && !EncoderAssistant::lossless(m_profileModel->getSelectedEncoderFromCurrentIndex())) {
        KMessageBox::error(this,
                           i18n("The profile creates an image, which requires a lossless encoder (WAVE or FLAC). Please change the profile."),
                           i18n("Image rip"));
        return;
    }

    if ((m_profileModel->isSelectedEncoderWithEmbedCover()
         || (m_profileModel->data(m_profileModel->index(m_profileModel->currentProfileRow(), PROFILE_MODEL_COLUMN_SC_INDEX)).toBool()))
        && m_cddaModel->cover().isNull()) {
        if (KMessageBox::warningTwoActions(this,
                                           i18n("No cover was set. Do you really want to continue?"),
                                           i18n("Cover is empty"),
                                           KStandardGuiItem::cont(),
                                           KStandardGuiItem::cancel(),
                                           "empty_cover_warn")
            == KMessageBox::SecondaryAction)
            return;
    }

    {
        ExtractingProgressDialog dialog(m_profileModel, m_cddaModel, m_encoders, m_discController->currentDrive(), m_discController->currentDriveUdi(), this);
        dialog.setWindowModality(Qt::ApplicationModal);
        dialog.exec();
    }

    if (Preferences::ejectCDTray())
        m_discController->eject();
}

void MainWindow::configure()
{
    SettingsDialog dialog(this, "settings", Preferences::self());

    KPageWidgetItem *generalPage = dialog.addPage(new generalSettingsWidget(), i18n("General settings"));
    generalPage->setIcon(QApplication::windowIcon());

    KPageWidgetItem *devicePage = dialog.addDevicePage(new deviceWidget(m_discController), i18n("Device settings"));
    devicePage->setIcon(QIcon::fromTheme("drive-optical"));

    KPageWidgetItem *profilePage = dialog.addPage(new profileWidget(m_profileModel), i18n("Profiles"));
    profilePage->setIcon(QIcon::fromTheme("document-multiple"));

    dialog.exec();

    // the current profile may have been removed or switched to a missing encoder
    m_profileModel->ensureAvailableCurrentProfile();
}

// ---------------------------------------------------------------------------
// track list editing
// ---------------------------------------------------------------------------

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
        m_cddaModel->splitTrackTitles(divider);
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

    m_cddaModel->swapArtistAndAlbum();
    m_cddaModel->swapTrackArtistsAndTitles();
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

    m_cddaModel->capitalizeAlbum();
    m_cddaModel->capitalizeTracks();
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

    m_cddaModel->setTrackArtistsFromAlbum();
}

// ---------------------------------------------------------------------------
// track selection
// ---------------------------------------------------------------------------

void MainWindow::toggle(const QModelIndex &idx)
{
    if (m_imageMode)
        return;
    if (idx.isValid() && (idx.column() == Audex::CDInfoModel::RipColumn)) {
        const int number = m_cddaModel->trackForRow(idx.row());
        if (number >= 0 && m_cddaModel->cdInfo().isAudioTrack(number))
            m_cddaModel->setSelected(number, !m_cddaModel->isSelected(number));
    }
}

void MainWindow::select_all()
{
    if (m_imageMode)
        return;
    m_cddaModel->selectAll();
}

void MainWindow::select_none()
{
    if (m_imageMode)
        return;
    m_cddaModel->selectNone();
}

void MainWindow::invert_selection()
{
    if (m_imageMode)
        return;
    m_cddaModel->invertSelection();
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

// ---------------------------------------------------------------------------
// drives and disc
// ---------------------------------------------------------------------------

void MainWindow::current_drive_updated_from_ui(int index)
{
    const QString udi = m_driveComboBox->itemData(index).toString();
    if (!udi.isEmpty())
        m_discController->setCurrentDrive(udi);
}

void MainWindow::drives_updated()
{
    if (!m_driveComboBox)
        return;
    const QList<DiscController::DriveInfo> drives = m_discController->drives();
    const QString current = m_discController->currentDriveUdi();
    const QSignalBlocker blocker(m_driveComboBox);
    m_driveComboBox->clear();
    for (const DiscController::DriveInfo &d : drives) {
        // the device node only to tell drives of the same model apart
        const auto sameName = [&d](const DiscController::DriveInfo &other) {
            return other.entry.displayName == d.entry.displayName;
        };
        const QString name = std::count_if(drives.cbegin(), drives.cend(), sameName) > 1 ? u"%1 (%2)"_s.arg(d.entry.displayName, d.entry.id)
                                                                                          : d.entry.displayName;
        switch (d.medium) {
        case DiscController::Medium::Audio:
            m_driveComboBox->addItem(QIcon::fromTheme(u"media-optical-audio"_s), i18nc("@item:inlistbox drive and its disc", "%1 – audio CD", name), d.udi);
            break;
        case DiscController::Medium::NoAudio:
            m_driveComboBox->addItem(QIcon::fromTheme(u"media-optical-data"_s), i18nc("@item:inlistbox drive and its disc", "%1 – no audio CD", name), d.udi);
            break;
        case DiscController::Medium::None:
            m_driveComboBox->addItem(QIcon::fromTheme(u"drive-optical"_s), i18nc("@item:inlistbox drive and its disc", "%1 – no disc", name), d.udi);
            break;
        }
        m_driveComboBox->setItemData(m_driveComboBox->count() - 1, d.entry.id, Qt::ToolTipRole);
    }
    if (drives.isEmpty()) {
        m_driveComboBox->addItem(QIcon::fromTheme(u"drive-optical"_s), i18n("No optical drive found"), QString());
        m_driveComboBox->setCurrentIndex(0);
    } else {
        const int idx = m_driveComboBox->findData(current);
        m_driveComboBox->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    m_driveComboBox->setEnabled(!drives.isEmpty());
}

void MainWindow::current_drive_updated()
{
    const QString udi = m_discController->currentDriveUdi();
    if (m_driveComboBox) {
        const QSignalBlocker blocker(m_driveComboBox);
        const int idx = m_driveComboBox->findData(udi);
        if (idx >= 0)
            m_driveComboBox->setCurrentIndex(idx);
    }
    actionCollection()->action("eject")->setEnabled(!udi.isEmpty());
}

void MainWindow::disc_detected(const Audex::DiscReadResult &result)
{
    stop_cover_fetch(); // a cover or a message of the previous disc
    m_cddaModel->setCDInfo(result.info); // selects all audio, the hidden track included
    m_htoaSilent = result.htoaSilent;
    m_savedSelection.clear(); // belonged to the previous disc
    // a silent hidden track is left out of track rips; an image always
    // contains the whole disc from its first sector
    if (m_htoaSilent && !m_imageMode)
        m_cddaModel->setSelected(0, false);
    m_cdtextProvider->setCandidates(result.info.toc(), result.discCandidates);

    enable_layout(true);

    m_cddaHeaderWidget->setHdcd(std::nullopt); // not checked (yet)
    m_cddaHeaderWidget->setCdg(std::nullopt);
    if (!Preferences::hdcdDetect())
        start_cdg_probe(); // otherwise after the HDCD probe: one after the other
    if (Preferences::hdcdDetect()) {
        if (m_hdcdWatcher.isRunning())
            m_hdcdWatcher.cancel();
        const Audex::DriveEntry drive = m_discController->currentDrive();
        const Audex::Cdda::Toc toc = result.info.toc();
        m_hdcdWatcher.setFuture(QtConcurrent::run(Audex::driveThreadPool(), [drive, toc]() -> std::optional<Audex::Hdcd::Result> {
            Audex::OpenedReader opened = Audex::openReader(drive);
            if (!opened.reader)
                return std::nullopt;
            const Audex::Hdcd::Result r = Audex::Hdcd::probe(*opened.reader, toc);
            opened.release();
            return r;
        }));
    }

    resizeColumns();

    if (Preferences::cddbLookupAuto()) {
        qDebug() << "Performing metadata auto lookup";
        QTimer::singleShot(0, this, [this]() {
            start_metadata_lookup(preferredProviderId(), true);
        });
    }

    update_layout();
}

void MainWindow::disc_removed()
{
    if (m_lookupId > 0 && m_lookup->isRunning(m_lookupId))
        m_lookup->cancel(m_lookupId);
    stop_cover_fetch();

    m_cddaModel->clear();
    m_cdtextProvider->clear();
    m_htoaSilent = false;
    m_savedSelection.clear();

    m_cddaHeaderWidget->setHdcd(std::nullopt);
    m_cddaHeaderWidget->setCdg(std::nullopt);

    enable_layout(false);
    update_layout();
}

void MainWindow::update_placeholder()
{
    using State = DiscController::State;
    const QString drive = m_discController->currentDrive().displayName;
    switch (m_discController->state()) {
    case State::Ready:
        m_trackStack->setCurrentWidget(m_cddaTreeView);
        return;
    case State::NoDrive:
        m_placeholder->setMessage(u"drive-optical"_s, i18n("No optical drive found"), i18n("Connect a CD drive or switch it on."));
        break;
    case State::NoDisc:
        m_placeholder->setMessage(u"drive-optical"_s, i18n("No disc in %1", drive), i18n("Insert an audio CD to rip it."), DiscPlaceholder::OpenTray);
        break;
    case State::NoAudio:
        m_placeholder->setMessage(u"media-optical-data"_s,
                                  i18n("No audio CD in %1", drive),
                                  i18n("The disc in this drive has no audio tracks."),
                                  DiscPlaceholder::Eject);
        break;
    case State::Reading:
        m_placeholder->setMessage(u"media-optical-audio"_s, i18n("Reading the CD…"), drive);
        break;
    case State::Failed: {
        DiscPlaceholder::Buttons buttons = DiscPlaceholder::Retry | DiscPlaceholder::Eject;
        if (!m_discController->failureDetails().isEmpty())
            buttons |= DiscPlaceholder::Details;
        m_placeholder->setMessage(u"dialog-error"_s, i18n("The CD in %1 cannot be read", drive), m_discController->failureMessage(), buttons);
        break;
    }
    }
    m_trackStack->setCurrentWidget(m_placeholder);
}

void MainWindow::hdcd_probe_finished()
{
    // a result that arrives after the disc was removed belongs to no disc
    if (m_hdcdWatcher.isCanceled() || m_hdcdWatcher.future().resultCount() == 0 || m_cddaModel->isEmpty())
        return;
    start_cdg_probe();
    const std::optional<Audex::Hdcd::Result> r = m_hdcdWatcher.result();
    if (!r)
        return; // not checked: the drive could not be opened
    if (r->detected)
        qDebug() << "HDCD detected: peak extend" << r->peakExtend << ", transient filter" << r->transientFilter << ", packets" << r->packets;
    m_cddaHeaderWidget->setHdcd(r->detected);
}

// A few seconds of the raw sub-channel: CD+G graphics on the disc?
void MainWindow::start_cdg_probe()
{
    if (!Preferences::cdgDetect() || m_cddaModel->isEmpty())
        return;
    const Audex::DriveEntry drive = m_discController->currentDrive();
    // a drive that cannot deliver it is not asked (some only answer after a timeout)
    if (DeviceSettings::features(m_discController->currentDriveUdi()).rwSubchannel == Audex::Rip::Feature::No)
        return;
    if (m_cdgWatcher.isRunning())
        m_cdgWatcher.cancel();
    const Audex::Cdda::Toc toc = m_cddaModel->cdInfo().toc();
    m_cdgWatcher.setFuture(QtConcurrent::run(Audex::driveThreadPool(), [drive, toc]() -> std::optional<bool> {
        Audex::OpenedReader opened = Audex::openReader(drive);
        if (!opened.reader)
            return std::nullopt;
        const bool found = Audex::Cdda::detectCdg(*opened.reader, toc);
        opened.release();
        return found;
    }));
}

void MainWindow::cdg_probe_finished()
{
    if (m_cdgWatcher.isCanceled() || m_cdgWatcher.future().resultCount() == 0 || m_cddaModel->isEmpty())
        return;
    m_cddaModel->setCdg(m_cdgWatcher.result()); // the rip reads CD+G only if found
    m_cddaHeaderWidget->setCdg(m_cdgWatcher.result());
}

// ---------------------------------------------------------------------------
// metadata and cover
// ---------------------------------------------------------------------------

void MainWindow::start_metadata_lookup(const QString &providerId, bool automatic)
{
    if (m_cddaModel->isEmpty())
        return;
    if (m_lookupId > 0 && m_lookup->isRunning(m_lookupId))
        return;

    m_lookupError.clear();
    m_lookupAuto = automatic;

    QStringList providers;
    if (!providerId.isEmpty())
        providers << providerId;

    // read from the settings on every lookup: the dialog may have changed it
    if (m_gnudbProvider) {
        m_gnudbProvider->setEmail(Preferences::gnudbEmail());
        // gnudb refuses requests without a contact address. The automatic
        // lookup says so once instead of failing at every disc, a lookup the
        // user started every time.
        if (providerId == u"gnudb"_s && !m_gnudbProvider->isAvailable()) {
            KMessageBox::information(this,
                                     i18n("gnudb requires an email address with every request. Please enter one in the settings."),
                                     i18n("Metadata Lookup"),
                                     automatic ? QStringLiteral("gnudb_email_missing") : QString());
            return;
        }
    }

    actionCollection()->action("cddbfetch")->setEnabled(false);
    actionCollection()->action("cddbfetch_cdtext")->setEnabled(false);
    actionCollection()->action("cddbfetch_musicbrainz")->setEnabled(false);
    actionCollection()->action("cddbfetch_gnudb")->setEnabled(false);
    m_lookupId = m_lookup->start(m_cddaModel->cdInfo(), providers);
}

void MainWindow::lookup_finished(int lookupId, const Audex::MetadataCandidates &candidates)
{
    if (lookupId != m_lookupId)
        return;

    actionCollection()->action("cddbfetch")->setEnabled(m_layoutEnabled);
    actionCollection()->action("cddbfetch_cdtext")->setEnabled(m_layoutEnabled);
    actionCollection()->action("cddbfetch_musicbrainz")->setEnabled(m_layoutEnabled);
    actionCollection()->action("cddbfetch_gnudb")->setEnabled(m_layoutEnabled);

    if (candidates.isEmpty()) {
        if (!m_lookupError.isEmpty()) {
            ErrorDialog::show(this,
                              i18n("The metadata lookup failed, with the following error:\n%1", m_lookupError),
                              QString(),
                              i18n("Metadata Lookup Failure"));
        } else if (!m_lookupAuto) {
            KMessageBox::information(this, i18n("No metadata found for this disc."), i18n("Metadata Lookup"));
        }
        return;
    }

    int selected = 0;
    if (candidates.count() > 1) {
        MetadataCandidateDialog dialog(candidates, this);
        if (dialog.exec() != QDialog::Accepted)
            return;
        selected = dialog.selectedCandidate();
        if (selected < 0)
            return;
    }

    const Audex::MetadataCandidate candidate = candidates.at(selected);
    m_cddaModel->applyCandidate(candidate);
    update_layout();

    // fetch the cover if the candidate knows one
    if (!m_lookupAuto || Preferences::coverLookupAuto())
        start_cover_fetch(candidate);
    else
        stop_cover_fetch(); // a result for the previous candidate would not fit
}

// The release's own cover, else (if allowed) the one chosen for its release group
void MainWindow::start_cover_fetch(const Audex::MetadataCandidate &candidate)
{
    stop_cover_fetch();
    m_coverCandidate = candidate;

    for (const Audex::MetadataCandidate::Cover &cover : candidate.covers) {
        if (cover.releaseGroup && !Preferences::coverReleaseGroupFallback())
            continue;
        PendingCover pending;
        pending.url = cover.url;
        pending.page = cover.page;
        pending.origin = cover.releaseGroup
            ? i18n("Cover Art Archive: the cover chosen for all releases of “%1”; this release has none of its own.", candidate.description())
            : i18n("Cover Art Archive: front cover of the release “%1”.", candidate.description());
        m_coverQueue.append(pending);
    }

    if (m_coverQueue.isEmpty()) {
        const QString detail = candidate.covers.isEmpty()
            ? i18n("%1 offers no cover for this album.", candidate.providerName)
            : i18n("Only the release group has a cover, and its use is turned off in the settings.");
        m_cddaHeaderWidget->setCoverState(CDDAHeaderWidget::CoverState::NotFound, detail);
        return;
    }
    m_cddaHeaderWidget->setCoverState(CDDAHeaderWidget::CoverState::Loading);
    fetch_next_cover();
}

void MainWindow::stop_cover_fetch()
{
    if (m_coverFetchId > 0 && m_coverFetcher->isRunning(m_coverFetchId))
        m_coverFetcher->cancel(m_coverFetchId);
    m_coverFetchId = 0;
    m_coverQueue.clear();
    if (m_cddaHeaderWidget)
        m_cddaHeaderWidget->setCoverState(CDDAHeaderWidget::CoverState::Idle);
}

void MainWindow::fetch_next_cover()
{
    if (m_coverQueue.isEmpty())
        return;
    m_coverFetching = m_coverQueue.takeFirst();
    m_coverFetchId = m_coverFetcher->fetch(m_coverFetching.url, preferredCoverSize());
}

void MainWindow::lookup_provider_failed(int lookupId, const QString &providerId, const QString &error)
{
    Q_UNUSED(providerId);
    if (lookupId != m_lookupId)
        return;
    m_lookupError = error;
}

void MainWindow::cover_fetch_finished(int requestId, const Audex::Metadata::CoverArt &cover, const QString &error)
{
    if (requestId != m_coverFetchId)
        return;
    m_coverFetchId = 0;

    // no dialog: the header shows what happened, the tool tip the details
    using State = CDDAHeaderWidget::CoverState;
    if (!cover.isNull()) {
        Audex::Metadata::CoverArt shown = cover;
        shown.origin = m_coverFetching.origin;
        shown.originPage = m_coverFetching.page;
        m_cddaHeaderWidget->setCoverState(State::Idle);
        m_cddaModel->setCover(shown);
    } else if (!error.isEmpty()) {
        m_coverQueue.clear();
        m_cddaHeaderWidget->setCoverState(State::Failed, error);
    } else if (!m_coverQueue.isEmpty()) {
        fetch_next_cover(); // this one does not exist: the next source
    } else {
        m_cddaHeaderWidget->setCoverState(State::NotFound, i18n("No cover was found for “%1”.", m_coverCandidate.description()));
    }
}

// ---------------------------------------------------------------------------
// profile and output mode
// ---------------------------------------------------------------------------

void MainWindow::current_profile_updated_from_ui(int row)
{
    if (row < 0)
        return;

    const QModelIndex source = m_profileFilter->mapToSource(m_profileFilter->index(row, PROFILE_MODEL_COLUMN_NAME_INDEX));
    if (source.isValid()) {
        {
            const QSignalBlocker blocker(m_profileModel);
            m_profileModel->setRowAsCurrentProfileIndex(source.row());
        }
        profileChanged();
    }
}

void MainWindow::update_profile_action(int index)
{
    if (index == -1) {
        if (m_layoutEnabled) {
            actionCollection()->action("profile_label")->setEnabled(false);
            actionCollection()->action("profile")->setEnabled(false);
        }
    } else {
        if (m_layoutEnabled) {
            actionCollection()->action("profile_label")->setEnabled(true);
            actionCollection()->action("profile")->setEnabled(true);
        }
        m_profileComboBox->setCurrentIndex(
            m_profileFilter->mapFromSource(m_profileModel->index(m_profileModel->getRowByIndex(index), PROFILE_MODEL_COLUMN_NAME_INDEX)).row());
    }
}

void MainWindow::update_profile_action()
{
    // When the Profile model emits 'reset' the profile combo clears its current settings.
    // Therefore, we need to try and reset these...
    if (m_profileComboBox->currentText().isEmpty()) {
        m_profileComboBox->setCurrentIndex(
            m_profileFilter->mapFromSource(m_profileModel->index(m_profileModel->currentProfileRow(), PROFILE_MODEL_COLUMN_NAME_INDEX)).row());
    }

    if (m_layoutEnabled) {
        actionCollection()->action("profile_label")->setEnabled(m_profileModel->rowCount() > 0);
        actionCollection()->action("profile")->setEnabled(m_profileModel->rowCount() > 0);
    }
}

void MainWindow::profileChanged()
{
    const int row = m_profileModel->currentProfileRow();
    m_profileComboBox->setToolTip(m_profileModel->data(m_profileModel->index(row, PROFILE_MODEL_COLUMN_NAME_INDEX), Qt::ToolTipRole).toString());
    applyImageMode(m_profileModel->isImage(row));
    updateSelectionActionStates();
}

void MainWindow::applyImageMode(bool image)
{
    if (m_imageMode == image)
        return;
    m_imageMode = image;

    // An image always contains the whole disc, the hidden track included
    // (the cue sheet then starts track 1 with INDEX 00 at 00:00:00). The
    // previous selection is restored when switching back; for a disc inserted
    // in image mode there is none, then the default of track rips applies.
    const QList<int> tracks = m_cddaModel->cdInfo().audioTrackNumbers(true);
    if (image) {
        m_savedSelection = m_cddaModel->selectedTracks();
        for (const int number : tracks)
            m_cddaModel->setSelected(number, true);
        m_cddaModel->setSelectionLocked(true);
    } else {
        m_cddaModel->setSelectionLocked(false);
        for (const int number : tracks) {
            const bool byDefault = !(number == 0 && m_htoaSilent);
            m_cddaModel->setSelected(number, m_savedSelection.isEmpty() ? byDefault : m_savedSelection.contains(number));
        }
        m_savedSelection.clear();
    }

    // nothing to choose in an image: no rip column
    m_cddaTreeView->setColumnHidden(Audex::CDInfoModel::RipColumn, image);

    updateSelectionActionStates();
}

void MainWindow::updateProfileMessage()
{
    // one hint per missing encoder plugin that a profile needs
    QStringList reasons;
    for (int row = 0; row < m_profileModel->rowCount(); ++row) {
        const QString reason = m_profileModel->unavailableReason(row);
        if (!reason.isEmpty() && !reasons.contains(reason))
            reasons << reason;
    }

    if (reasons.isEmpty()) {
        m_profileMessage->animatedHide();
        m_profileMessage->setText(QString());
        return;
    }

    // shown again only if something changed after the user closed it
    const QString text = i18n("Profiles that need a missing encoder plugin are disabled. %1", reasons.join(u' '));
    if (text != m_profileMessage->text()) {
        m_profileMessage->setText(text);
        m_profileMessage->animatedShow();
    }
}

// ---------------------------------------------------------------------------
// layout and configuration
// ---------------------------------------------------------------------------

void MainWindow::update_layout()
{
    if (!m_cddaModel->variousArtists()) {
        m_cddaTreeView->hideColumn(Audex::CDInfoModel::ArtistColumn);
    } else {
        m_cddaTreeView->showColumn(Audex::CDInfoModel::ArtistColumn);
    }
    resizeColumns();
    updateSelectionActionStates();
}

void MainWindow::enable_layout(bool enabled)
{
    m_layoutEnabled = enabled;
    m_cddaTreeView->setEnabled(enabled);
    m_cddaHeaderDock->setEnabled(enabled);
    m_cddaHeaderWidget->setEnabled(enabled);
    actionCollection()->action("profile_label")->setEnabled((m_profileModel->rowCount() > 0) && (enabled));
    m_profileComboBox->setEnabled((m_profileModel->rowCount() > 0) && (enabled));
    actionCollection()->action("profile")->setEnabled((m_profileModel->rowCount() > 0) && (enabled));
    actionCollection()->action("cddbfetch")->setEnabled(enabled);
    actionCollection()->action("cddbfetch_cdtext")->setEnabled(enabled);
    actionCollection()->action("cddbfetch_musicbrainz")->setEnabled(enabled);
    actionCollection()->action("cddbfetch_gnudb")->setEnabled(enabled);
    actionCollection()->action("edit")->setEnabled(enabled);
    actionCollection()->action("eject")->setEnabled(enabled || !m_discController->currentDriveUdi().isEmpty());
    actionCollection()->action("splittitles")->setEnabled(enabled);
    actionCollection()->action("swapartistsandtitles")->setEnabled(enabled);
    actionCollection()->action("capitalize")->setEnabled(enabled);
    actionCollection()->action("autofillartists")->setEnabled(enabled);
    updateSelectionActionStates();
}

void MainWindow::resizeColumns()
{
    for (int i = 0; i < Audex::CDInfoModel::ColumnCount; ++i)
        m_cddaTreeView->resizeColumnToContents(i);
}

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

void MainWindow::updateSelectionActionStates()
{
    const int selected = m_cddaModel->selectedCount();
    const bool editable = m_layoutEnabled && !m_imageMode;
    const bool usable = m_profileModel->isAvailable(m_profileModel->currentProfileRow());
    actionCollection()->action("rip")->setEnabled(m_layoutEnabled && selected > 0 && usable);
    actionCollection()->action("selectall")->setEnabled(editable && selected < audioTrackCount());
    actionCollection()->action("selectnone")->setEnabled(editable && selected > 0);
    actionCollection()->action("invertselection")->setEnabled(editable);
}

int MainWindow::audioTrackCount() const
{
    return m_cddaModel->cdInfo().audioTrackNumbers(true).count();
}
