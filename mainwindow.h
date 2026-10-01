/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QFutureWatcher>
#include <QList>
#include <QPointer>
#include <QString>

#include <KXmlGuiWindow>

#include <memory>
#include <optional>

#include "core/hdcd.h" // Audex::Hdcd::Result - must be complete for m_hdcdWatcher
#include "metadata/lookup.h" // Audex::MetadataCandidates (slot signature)
#include "online/coverartfetcher.h" // Audex::Metadata::CoverArt (slot signature)

class QDockWidget;
class QLabel;
class QModelIndex;
class QNetworkAccessManager;
class QTreeView;

class KComboBox;
class KMessageWidget;

class CDDAHeaderWidget;
class DiscController;
class ProfileFilterModel;
class ProfileModel;

namespace Audex
{
class CDInfoModel;
class PrecomputedProvider;
struct DiscReadResult;

namespace Encoding
{
class EncoderRegistry;
}
}

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool isValid() const;

private Q_SLOTS:
    // toolbar / menu actions
    void eject();
    void fetch_metadata();
    void fetch_metadata_cdtext();
    void fetch_metadata_musicbrainz();
    void edit();
    void rip();
    void configure();

    // track list editing
    void split_titles();
    void swap_artists_and_titles();
    void capitalize();
    void auto_fill_artists();

    // track selection
    void toggle(const QModelIndex &idx);
    void select_all();
    void select_none();
    void invert_selection();
    void cdda_context_menu(const QPoint &pos);

    // drives and disc
    void current_drive_updated_from_ui(int index);
    void drives_updated();
    void current_drive_updated();
    void disc_detected(const Audex::DiscReadResult &result);
    void disc_removed();
    void disc_failed(const QString &message, const QString &details);
    void hdcd_probe_finished();
    void start_cdg_probe();
    void cdg_probe_finished();

    // metadata and cover
    void lookup_finished(int lookupId, const Audex::MetadataCandidates &candidates);
    void lookup_provider_failed(int lookupId, const QString &providerId, const QString &error);
    void cover_fetch_finished(int requestId, const Audex::Metadata::CoverArt &cover, const QString &error);
    void start_cover_fetch(const Audex::MetadataCandidate &candidate);
    void fetch_next_cover();

    // profile
    void current_profile_updated_from_ui(int row);
    void update_profile_action(int index);
    void update_profile_action();

    // layout and configuration
    void update_layout();
    void enable_layout(bool enabled);
    void resizeColumns();

private:
    bool firstStart();
    void setup_actions();
    void setup_layout();

    void start_metadata_lookup(const QString &providerId, bool automatic);
    void profileChanged();
    void applyImageMode(bool image);
    void updateProfileMessage();
    void updateSelectionActionStates();
    int audioTrackCount() const;

    // models
    QPointer<ProfileModel> m_profileModel;
    QPointer<ProfileFilterModel> m_profileFilter;
    QPointer<Audex::CDInfoModel> m_cddaModel;

    // backend
    std::shared_ptr<Audex::Encoding::EncoderRegistry> m_encoders;
    QPointer<DiscController> m_discController;
    QPointer<QNetworkAccessManager> m_network;
    QPointer<Audex::MetadataLookup> m_lookup;
    QPointer<Audex::PrecomputedProvider> m_cdtextProvider;
    QPointer<Audex::CoverArtFetcher> m_coverFetcher;
    QFutureWatcher<std::optional<Audex::Hdcd::Result>> m_hdcdWatcher; // empty: the drive could not be opened
    QFutureWatcher<std::optional<bool>> m_cdgWatcher; // CD+G; runs after the HDCD probe, empty: not checked

    // widgets
    QPointer<QTreeView> m_cddaTreeView;
    QPointer<QDockWidget> m_cddaHeaderDock;
    QPointer<CDDAHeaderWidget> m_cddaHeaderWidget;
    QPointer<KMessageWidget> m_profileMessage;
    QPointer<KComboBox> m_driveComboBox;
    QPointer<QLabel> m_profileLabel;
    QPointer<KComboBox> m_profileComboBox;

    // metadata lookup state
    int m_lookupId = 0;
    QString m_lookupError;
    bool m_lookupAuto = false;
    int m_coverFetchId = 0;
    struct PendingCover {
        QUrl url;
        QUrl page;
        QString origin; // for the user
    };
    PendingCover m_coverFetching; // the one m_coverFetchId loads
    QList<PendingCover> m_coverQueue; // to try if it does not exist

    // ui state
    bool m_layoutEnabled = false;
    bool m_imageMode = false;
    QList<int> m_savedSelection; // track selection before image mode forced all tracks
    bool m_htoaSilent = false; // the hidden track one audio of the disc is silence
    bool m_valid = false; // set once the constructor has completed
};
