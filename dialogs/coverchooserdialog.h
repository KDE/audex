/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "metadata/metadata.h"
#include "online/coverartfetcher.h"

#include <QDialog>
#include <QHash>
#include <QStringList>

class QDialogButtonBox;
class QLabel;
class QListWidget;
class QNetworkAccessManager;

// Size of fetched covers as set in the general settings
Audex::CoverArtFetcher::Size preferredCoverSize();

// Shows all images the Cover Art Archive has for the release (and for the
// release chosen for its release group) and downloads the one picked, in the
// size set in the general settings.
class CoverChooserDialog : public QDialog
{
    Q_OBJECT

public:
    // album: MusicBrainz release and release group, artist and title
    explicit CoverChooserDialog(const Audex::Metadata::Album &album, QWidget *parent = nullptr);

    // The album is known to MusicBrainz: there is something to choose from
    static bool canChoose(const Audex::Metadata::Album &album);

    // valid once the dialog was accepted, with origin and page set
    Audex::Metadata::CoverArt cover() const
    {
        return m_cover;
    }

public Q_SLOTS:
    void accept() override;

private Q_SLOTS:
    void listingFinished(int requestId, const Audex::CoverArtListing &listing, const QString &error);
    void imageFinished(int requestId, const Audex::Metadata::CoverArt &cover, const QString &error);

private:
    struct Entry {
        Audex::CoverArtImage image;
        QUrl page; // the cover art of its release at MusicBrainz
        bool otherRelease = false; // from the release group, not this release
    };

    void updateStatus();
    QString originText(const Entry &entry) const;

    QNetworkAccessManager *m_network = nullptr;
    Audex::CoverArtFetcher *m_fetcher = nullptr;
    QListWidget *m_list = nullptr;
    QLabel *m_status = nullptr;
    QDialogButtonBox *m_buttons = nullptr;

    QString m_title; // "Artist – Album"
    QList<Entry> m_entries;
    int m_releaseItems = 0; // images of this release come first
    QHash<int, bool> m_listings; // running listing requests -> release group
    QHash<int, int> m_thumbnails; // running thumbnail requests -> entry
    int m_downloadId = 0;
    int m_downloadEntry = -1;
    QStringList m_errors;
    Audex::Metadata::CoverArt m_cover;
};
