/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractTableModel>
#include <QImage>
#include <QImageReader>
#include <QInputDialog>
#include <QMimeDatabase>
#include <QModelIndexList>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QVariant>

#include <KLocalizedString>

#include "datatypes/cdda.h"

#define CDINFO_MODEL_COLUMN_RIP_LABEL i18n("Rip")
#define CDINFO_MODEL_COLUMN_TRACKNUMBER_LABEL i18n("#")
#define CDINFO_MODEL_COLUMN_ARTIST_LABEL i18n("Artist")
#define CDINFO_MODEL_COLUMN_TITLE_LABEL i18n("Title")
#define CDINFO_MODEL_COLUMN_LENGTH_LABEL i18n("Length")

namespace Audex
{

class CDDAModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columms {
        COLUMN_RIP_INDEX = 0,
        COLUMN_TRACKNUMBER_INDEX,
        COLUMN_ARTIST_INDEX,
        COLUMN_TITLE_INDEX,
        COLUMN_LENGTH_INDEX,
        COLUMN_COUNT
    };

    explicit CDDAModel(QObject *parent = nullptr);
    ~CDDAModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setArtist(const QString &artist);
    const QString artist() const;
    void setAlbum(const QString &album);
    const QString album() const;
    void setCDDBCategory(const QString &cat);
    const QString cddbCategory() const;
    void setGenre(const QString &genre);
    const QString genre() const;
    void setYear(const QString &year);
    const QString year() const;
    void setComment(const QStringList &comment);
    const QStringList comment() const;
    void setDiscNumber(const int number);
    int discNumber() const;
    void setDiscCount(const int count);
    int discCount() const;
    void setTrackNumberOffset(const int offset);
    int trackNumberOffset() const;
    int guessMultiDisc(QString &newTitle) const;
    void setMultiDisc(const bool multi);
    bool isMultiDisc() const;
    bool guessVarious() const;
    void setVarious(const bool various);
    bool isVarious() const;
    bool isValidAudioTrack(const int tracknumber) const;
    void setCustomData(const QString &type, const QVariant &data);
    const QVariant customData(const QString &type) const;
    void setCustomTrackData(const int tracknumber, const QString &type, const QVariant &data);
    const QVariant customTrackData(const int tracknumber, const QString &type) const;
    void setCover(const QImage &cover);
    const QImage cover() const;
    bool loadCoverFromFile(const QString &filename);
    bool saveCoverToFile(const QString &filename);
    bool coverExists() const;
    void removeCover();
    void swapArtistAndTitleOfTracks();
    void swapArtistAndAlbum();
    void splitTitleOfTracks(const QString &divider);
    void capitalizeTracks();
    void capitalizeHeader();
    void setTitleArtistsFromHeader();

    inline const TracknumberSet &selectedTracks() const
    {
        return sel_tracks;
    }
    void toggle(int row);
    bool isTrackInSelection(int tracknumber) const;
    void invertSelection();
    void selectAll();
    void selectNone();

    bool isModified() const;
    void confirm();

    bool hasMetadata() const;

public Q_SLOTS:
    void update(const CDDA &cdda);
    void clear();

Q_SIGNALS:
    void metadataChanged(const Metadata::Metadata &metadata);

    void hasSelection(bool has_selection);
    void selectionChanged(const int num_selected);

private:
    CDDA cdda;

    QSet<int> sel_tracks;
    void p_toggle(const int track);
};

}
