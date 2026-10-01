/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractTableModel>
#include <QImage>
#include <QList>
#include <QSet>

#include "metadata/cdinfo.h"

namespace Audex
{

// Table model on top of CDInfo: one row per disc entry (hidden track first,
// data tracks included but disabled). Album level values are exposed as
// properties; only track data goes through the item interface.
//
// Changes are reported precisely (dataChanged for the affected cells); a
// model reset happens only when a different disc is set.
class CDInfoModel : public QAbstractTableModel
{
    Q_OBJECT

    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY albumChanged)
    Q_PROPERTY(QString album READ album WRITE setAlbum NOTIFY albumChanged)
    Q_PROPERTY(QString year READ year WRITE setYear NOTIFY albumChanged)
    Q_PROPERTY(QString genre READ genre WRITE setGenre NOTIFY albumChanged)
    Q_PROPERTY(QString comment READ comment WRITE setComment NOTIFY albumChanged)
    Q_PROPERTY(int discNumber READ discNumber WRITE setDiscNumber NOTIFY albumChanged)
    Q_PROPERTY(int discCount READ discCount WRITE setDiscCount NOTIFY albumChanged)
    Q_PROPERTY(int trackNumberOffset READ trackNumberOffset WRITE setTrackNumberOffset NOTIFY albumChanged)
    Q_PROPERTY(bool variousArtists READ variousArtists WRITE setVariousArtists NOTIFY albumChanged)

public:
    enum Column {
        RipColumn = 0,
        TrackNumberColumn,
        ArtistColumn,
        TitleColumn,
        LengthColumn,
        ColumnCount
    };
    Q_ENUM(Column)

    enum Role {
        TrackNumberRole = Qt::UserRole + 1, // TOC track number, 0 = hidden track
        IsAudioRole,
        IsHiddenRole,
        FirstSectorRole,
        SectorCountRole,
        IsPlaceholderRole, // the title column shows a generated text
    };
    Q_ENUM(Role)

    explicit CDInfoModel(QObject *parent = nullptr);

    // --- disc ---
    const CDInfo &cdInfo() const
    {
        return m_info;
    }
    void setCDInfo(const CDInfo &info); // resets the model, selects all audio entries, confirms
    void setCdg(std::optional<bool> found) // result of the CD+G detection, no reset
    {
        m_info.setCdg(found);
    }
    void clear();
    bool isEmpty() const
    {
        return m_info.isEmpty();
    }

    int rowForTrack(int number) const; // -1 if unknown
    int trackForRow(int row) const; // -1 if invalid

    // --- item model ---
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // --- album values ---
    QVariant albumValue(Metadata::Field field) const;
    bool setAlbumValue(Metadata::Field field, const QVariant &value);

    QString artist() const;
    void setArtist(const QString &value);
    QString album() const;
    void setAlbum(const QString &value);
    QString year() const;
    void setYear(const QString &value);
    QString genre() const;
    void setGenre(const QString &value);
    QString comment() const;
    void setComment(const QString &value);
    int discNumber() const;
    void setDiscNumber(int value);
    int discCount() const;
    void setDiscCount(int value);
    int trackNumberOffset() const;
    void setTrackNumberOffset(int value);
    bool variousArtists() const;
    void setVariousArtists(bool value);

    // --- track values ---
    QVariant trackValue(int number, Metadata::Field field) const;
    bool setTrackValue(int number, Metadata::Field field, const QVariant &value);

    // --- cover ---
    Metadata::CoverArt cover() const;
    QImage coverImage() const; // decoded once, null if there is no cover
    bool setCover(const Metadata::CoverArt &cover);
    bool loadCoverFromFile(const QString &path, QString *error = nullptr);
    bool saveCoverToFile(const QString &path, QString *error = nullptr) const;
    bool removeCover();

    // --- bulk edits ---
    void applyCandidate(const MetadataCandidate &candidate, Metadata::MergePolicy policy = Metadata::MergePolicy::Overwrite);
    void swapTrackArtistsAndTitles();
    void swapArtistAndAlbum();
    void splitTrackTitles(const QString &divider);
    void capitalizeTracks();
    void capitalizeAlbum();
    void setTrackArtistsFromAlbum(bool onlyEmpty = false);

    // --- selection (the "rip" check boxes) ---
    QList<int> selectedTracks() const; // ascending TOC numbers, 0 = hidden track
    int selectedCount() const
    {
        return int(m_selected.size());
    }
    bool isSelected(int number) const;
    void setSelected(int number, bool selected);
    void selectAll();
    void selectNone();
    void invertSelection();
    // Image rips: every audio entry is part of the image. The check boxes
    // cannot be toggled and are not shown (the main window hides their
    // column as well); setSelected() still works.
    void setSelectionLocked(bool locked);
    bool isSelectionLocked() const
    {
        return m_selectionLocked;
    }

    // --- modification state ---
    bool isModified() const;
    void confirm();

Q_SIGNALS:
    void albumChanged();
    void coverChanged();
    void metadataChanged(); // any metadata change, album or tracks
    void selectionChanged(int selectedCount);

private:
    bool isSelectable(int number) const;
    void emitTrackColumnsChanged(int firstColumn, int lastColumn);
    void emitSelectionChanged(const QSet<int> &before);
    void afterBulkEdit(bool albumMayHaveChanged);
    QString placeholderTitle(const DiscEntry &entry) const;

    CDInfo m_info;
    QList<DiscEntry> m_entries; // row order
    QSet<int> m_selected;
    bool m_selectionLocked = false;
    mutable QImage m_coverImage;
    mutable bool m_coverImageValid = false;
};

}
