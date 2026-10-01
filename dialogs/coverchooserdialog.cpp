/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "coverchooserdialog.h"

#include "preferences.h"

#include <KLocalizedString>

#include <QDialogButtonBox>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::StringLiterals;
using Audex::Metadata::Field;

namespace
{
constexpr int ThumbnailSize = 160; // shown; loaded as 250 pixel thumbnails
}

Audex::CoverArtFetcher::Size preferredCoverSize()
{
    switch (Preferences::coverSize()) {
    case Preferences::EnumCoverSize::Small:
        return Audex::CoverArtFetcher::Size::Small;
    case Preferences::EnumCoverSize::Medium:
        return Audex::CoverArtFetcher::Size::Medium;
    case Preferences::EnumCoverSize::Original:
        return Audex::CoverArtFetcher::Size::Original;
    default:
        return Audex::CoverArtFetcher::Size::Large;
    }
}

bool CoverChooserDialog::canChoose(const Audex::Metadata::Album &album)
{
    return !album.text(Field::MusicBrainzReleaseId).isEmpty() || !album.text(Field::MusicBrainzReleaseGroupId).isEmpty();
}

CoverChooserDialog::CoverChooserDialog(const Audex::Metadata::Album &album, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Choose Cover"));

    m_network = new QNetworkAccessManager(this);
    m_fetcher = new Audex::CoverArtFetcher(m_network, this);
    connect(m_fetcher, &Audex::CoverArtFetcher::listingFinished, this, &CoverChooserDialog::listingFinished);
    connect(m_fetcher, &Audex::CoverArtFetcher::finished, this, &CoverChooserDialog::imageFinished);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);

    m_list = new QListWidget(this);
    m_list->setViewMode(QListView::IconMode);
    m_list->setIconSize(QSize(ThumbnailSize, ThumbnailSize));
    m_list->setGridSize(QSize(ThumbnailSize + 40, ThumbnailSize + 60));
    m_list->setResizeMode(QListView::Adjust);
    m_list->setMovement(QListView::Static);
    m_list->setWordWrap(true);
    m_list->setUniformItemSizes(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    auto *source = new QLabel(i18n("Images from the <a href=\"https://coverartarchive.org/\">Cover Art Archive</a>, "
                                   "for this release and, marked as such, for the release chosen for all releases of the album."),
                              this);
    source->setWordWrap(true);
    source->setOpenExternalLinks(true);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(i18n("Use Cover"));
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &CoverChooserDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_list, &QListWidget::itemSelectionChanged, this, [this] {
        m_buttons->button(QDialogButtonBox::Ok)->setEnabled(m_list->currentItem() && m_downloadId == 0);
    });
    connect(m_list, &QListWidget::itemActivated, this, &CoverChooserDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_status);
    layout->addWidget(m_list, 1);
    layout->addWidget(source);
    layout->addWidget(m_buttons);
    resize(680, 520);

    const QString artist = album.text(Field::Artist);
    const QString title = album.text(Field::Album);
    m_title = artist.isEmpty() ? title : title.isEmpty() ? artist : u"%1 – %2"_s.arg(artist, title);

    const QString release = album.text(Field::MusicBrainzReleaseId);
    const QString group = album.text(Field::MusicBrainzReleaseGroupId);
    if (!release.isEmpty())
        m_listings.insert(m_fetcher->fetchListing(QUrl(u"https://coverartarchive.org/release/%1"_s.arg(release))), false);
    if (!group.isEmpty())
        m_listings.insert(m_fetcher->fetchListing(QUrl(u"https://coverartarchive.org/release-group/%1"_s.arg(group))), true);
    updateStatus();
}

void CoverChooserDialog::listingFinished(int requestId, const Audex::CoverArtListing &listing, const QString &error)
{
    if (!m_listings.contains(requestId))
        return;
    const bool group = m_listings.take(requestId);
    if (!error.isEmpty())
        m_errors << error;

    QUrl page = listing.release;
    if (page.isValid())
        page.setPath(page.path() + u"/cover-art"_s);

    for (const Audex::CoverArtImage &image : listing.images) {
        // the release group's release may be this release
        const bool known = std::any_of(m_entries.cbegin(), m_entries.cend(), [&image](const Entry &e) {
            return e.image.image == image.image;
        });
        if (known)
            continue;

        const int index = int(m_entries.size());
        m_entries.append(Entry{image, page, group});

        QString label = image.types.isEmpty() ? i18n("Image") : image.types.join(u", "_s);
        if (group)
            label += u'\n' + i18n("(other release)");
        auto *item = new QListWidgetItem(QIcon::fromTheme(u"image-x-generic"_s), label);
        item->setData(Qt::UserRole, index);
        QStringList tip;
        if (!image.comment.isEmpty())
            tip << image.comment;
        tip << (group ? i18n("From the release chosen for all releases of this album.") : i18n("From this release."));
        item->setToolTip(tip.join(u'\n'));
        if (group)
            m_list->addItem(item);
        else
            m_list->insertItem(m_releaseItems++, item);

        m_thumbnails.insert(m_fetcher->fetch(image.url(250)), index);
    }
    updateStatus();
}

void CoverChooserDialog::imageFinished(int requestId, const Audex::Metadata::CoverArt &cover, const QString &error)
{
    if (requestId == m_downloadId) {
        m_downloadId = 0;
        if (cover.isNull()) {
            m_errors << (error.isEmpty() ? i18n("The image is no longer available.") : error);
            m_buttons->button(QDialogButtonBox::Ok)->setEnabled(m_list->currentItem());
            updateStatus();
            return;
        }
        const Entry &entry = m_entries.at(m_downloadEntry);
        m_cover = cover;
        m_cover.origin = originText(entry);
        m_cover.originPage = entry.page;
        QDialog::accept();
        return;
    }

    if (!m_thumbnails.contains(requestId))
        return;
    const int index = m_thumbnails.take(requestId);
    QPixmap pixmap;
    if (!cover.isNull() && pixmap.loadFromData(cover.data)) {
        for (int row = 0; row < m_list->count(); ++row) {
            QListWidgetItem *item = m_list->item(row);
            if (item->data(Qt::UserRole).toInt() == index)
                item->setIcon(QIcon(pixmap.scaled(ThumbnailSize, ThumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
    }
}

void CoverChooserDialog::accept()
{
    const QListWidgetItem *item = m_list->currentItem();
    if (!item || m_downloadId != 0)
        return;
    m_downloadEntry = item->data(Qt::UserRole).toInt();
    // a direct image address: the thumbnail of the configured size, or the original
    m_downloadId = m_fetcher->fetch(m_entries.at(m_downloadEntry).image.url(int(preferredCoverSize())));
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    updateStatus();
}

void CoverChooserDialog::updateStatus()
{
    QString text;
    if (m_downloadId != 0)
        text = i18n("Downloading the cover...");
    else if (!m_listings.isEmpty())
        text = i18n("Looking up the covers at the Cover Art Archive...");
    else if (m_entries.isEmpty() && m_errors.isEmpty())
        text = i18n("The Cover Art Archive has no covers for this album.");
    else if (!m_entries.isEmpty())
        text = i18np("One image. Pick it to use it as the cover.", "%1 images. Pick one to use it as the cover.", m_entries.size());
    if (!m_errors.isEmpty())
        text += u' ' + m_errors.join(u' ');
    m_status->setText(text.trimmed());
}

QString CoverChooserDialog::originText(const Entry &entry) const
{
    const QString types = entry.image.types.isEmpty() ? i18n("untyped") : entry.image.types.join(u", "_s);
    return entry.otherRelease ? i18n("Cover Art Archive: %1 image of another release of “%2”.", types, m_title)
                              : i18n("Cover Art Archive: %1 image of the release “%2”.", types, m_title);
}
