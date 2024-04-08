/* AUDEX CDINFO EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdinfomodel.h"

namespace Audex
{

CDInfoModel::CDInfoModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    Q_UNUSED(parent);
}

CDInfoModel::~CDInfoModel()
{
}

int CDInfoModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return cd_info.toc().trackCount();
}

int CDInfoModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return COLUMN_COUNT;
}

QVariant CDInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if ((index.row() < 0) || (index.row() >= cd_info.toc().trackCount()))
        return QVariant();

    if (role == Qt::TextAlignmentRole)
        return int(Qt::AlignLeft | Qt::AlignVCenter);

    if (role == Qt::ForegroundRole) {
        if (!cd_info.toc().isAudioTrack(index.row() + 1))
            return QVariant(QColor(Qt::gray));
    }

    if ((role == Qt::DisplayRole) || (role == Qt::CheckStateRole && index.column() == COLUMN_RIP_INDEX) || role == Qt::EditRole) {
        switch (index.column()) {
        case COLUMN_RIP_INDEX:
            if (role == Qt::CheckStateRole)
                return isTrackInSelection(index.row() + 1) ? Qt::Checked : Qt::Unchecked;
            break;
        case COLUMN_TRACKNUMBER_INDEX: {
            int tracknumber_offset = cd_info.metadata().get(Metadata::TrackNumberOffset, 0).toInt();
            return index.row() + 1 + (tracknumber_offset > 1 ? tracknumber_offset : 0);
        }
        case COLUMN_LENGTH_INDEX: {
            if (cd_info.toc().isAudioTrack(index.row() + 1)) {
                int length_of_track = 0;
                if (!cd_info.toc().isEmpty())
                    length_of_track = cd_info.toc().lengthOfTrack(index.row() + 1);
                return QStringLiteral(u"%1:%2").arg(length_of_track / 60, 2, 10, QChar(u'0')).arg(length_of_track % 60, 2, 10, QChar(u'0'));
            }
            return i18n("%1 MiB", QString::number(cd_info.toc().sizeTrack(index.row() + 1), 'f', 2));
        }
        case COLUMN_ARTIST_INDEX:
            if (index.row() < cd_info.metadata().trackCount())
                return cd_info.metadata().track(index.row() + 1).get(Metadata::Artist).toString();
            break;
        case COLUMN_TITLE_INDEX:
            QString title;
            if (index.row() < cd_info.metadata().trackCount())
                title = cd_info.metadata().track(index.row() + 1).get(Metadata::Title).toString();

            if (title.isEmpty()) {
                if (!cd_info.toc().isAudioTrack(index.row() + 1))
                    return i18n("Data Track %1", index.row() + 1);
                else
                    return i18n("Track %1", index.row() + 1);
            }

            return title;
        }
    }

    return QVariant();
}

bool CDInfoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if ((index.row() < 0) || (index.row() >= cd_info.toc().trackCount()))
        return false;

    if (role == Qt::EditRole) {
        bool changed = false;
        switch (index.column()) {
        case COLUMN_ARTIST_INDEX:
            changed = cd_info.metadata().track(index.row() + 1).set(Metadata::Artist, value);
            break;
        case COLUMN_TITLE_INDEX:
            changed = cd_info.metadata().track(index.row() + 1).set(Metadata::Title, value);
            break;
        default:
            return false;
        }
        if (changed) {
            Q_EMIT dataChanged(index, index);
            Q_EMIT metadataChanged(cd_info.metadata());
        }
        return changed;
    }

    return false;
}

QVariant CDInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);

    if (orientation == Qt::Horizontal) {
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case COLUMN_RIP_INDEX:
                return CDINFO_MODEL_COLUMN_RIP_LABEL;
            case COLUMN_TRACKNUMBER_INDEX:
                return CDINFO_MODEL_COLUMN_TRACKNUMBER_LABEL;
            case COLUMN_LENGTH_INDEX:
                return CDINFO_MODEL_COLUMN_LENGTH_LABEL;
            case COLUMN_ARTIST_INDEX:
                return CDINFO_MODEL_COLUMN_ARTIST_LABEL;
            case COLUMN_TITLE_INDEX:
                return CDINFO_MODEL_COLUMN_TITLE_LABEL;
            default:
                ;
            }
            break;
        case Qt::TextAlignmentRole:
            return Qt::AlignLeft;
        default:
            ;
        }
    } else if (orientation == Qt::Vertical) {
        if (role == Qt::DisplayRole) {
            return QVariant(section + 1);
        }
    }

    return QVariant();
}

Qt::ItemFlags CDInfoModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled;
    if ((index.column() == COLUMN_ARTIST_INDEX) || (index.column() == COLUMN_TITLE_INDEX))
        flags |= Qt::ItemIsEditable;
    return flags;
}

void CDInfoModel::setArtist(const QString &artist)
{
    beginResetModel();
    if (cd_info.metadata().set(Metadata::Artist, artist))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

const QString CDInfoModel::artist() const
{
    return cd_info.metadata().get(Metadata::Artist).toString();
}

void CDInfoModel::setAlbum(const QString &album)
{
    beginResetModel();
    if (cd_info.metadata().set(Metadata::Album, album))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

const QString CDInfoModel::album() const
{
    return cd_info.metadata().get(Metadata::Album).toString();
}

void CDInfoModel::setCDDBCategory(const QString &cat)
{
    beginResetModel();
    if (cd_info.metadata().set(Metadata::CDDBCategory, cat))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

const QString CDInfoModel::cddbCategory() const
{
    return cd_info.metadata().get(Metadata::CDDBCategory).toString();
}

void CDInfoModel::setGenre(const QString &genre)
{
    beginResetModel();
    if (cd_info.metadata().set(Metadata::Genre, genre))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

const QString CDInfoModel::genre() const
{
    return cd_info.metadata().get(Metadata::Genre).toString();
}

void CDInfoModel::setYear(const QString &year)
{
    beginResetModel();
    if (cd_info.metadata().set(Metadata::Year, year))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

const QString CDInfoModel::year() const
{
    return cd_info.metadata().get(Metadata::Year).toString();
}

void CDInfoModel::setComment(const QStringList &comment)
{
    beginResetModel();
    if (cd_info.metadata().set(Metadata::Comment, comment))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

const QStringList CDInfoModel::comment() const
{
    return cd_info.metadata().get(Metadata::Comment).toStringList();
}

void CDInfoModel::setDiscnumber(const int number)
{
    beginResetModel();
    if (cd_info.metadata().set(Metadata::Discnumber, number))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

int CDInfoModel::discnumber() const
{
    if (!cd_info.metadata().isMultiDisc())
        return 0;
    return cd_info.metadata().get(Metadata::Discnumber).toInt();
}

void CDInfoModel::setTracknumberOffset(const int offset)
{
    beginResetModel();
    if (cd_info.metadata().set(Metadata::TrackNumberOffset, offset))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

int CDInfoModel::guessMultiDisc(QString &newTitle) const
{
    return cd_info.metadata().guessMultiDisc(newTitle);
}

void CDInfoModel::setMultiDisc(const bool multi)
{
    beginResetModel();
    if (cd_info.metadata().set(Metadata::MultiDisc, multi))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

bool CDInfoModel::isMultiDisc() const
{
    return cd_info.metadata().get(Metadata::MultiDisc).toBool();
}

void CDInfoModel::setCustomData(const QString &type, const QVariant &data)
{
    beginResetModel();
    if (cd_info.metadata().setCustom(type, data))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

bool CDInfoModel::guessVarious() const
{
    return cd_info.metadata().guessVarious();
}

void CDInfoModel::setVarious(const bool various)
{
    if (cd_info.metadata().set(Metadata::VariousArtists, various))
        Q_EMIT metadataChanged(cd_info.metadata());
}

bool CDInfoModel::isVarious() const
{
    return cd_info.metadata().get(Metadata::VariousArtists).toBool();
}

const QVariant CDInfoModel::customData(const QString &type) const
{
    return cd_info.metadata().getCustom(type);
}

bool CDInfoModel::isValidAudioTrack(const int tracknumber) const
{
    if (tracknumber < 1 && tracknumber > cd_info.toc().trackCount())
        return false;
    return cd_info.toc().isAudioTrack(tracknumber);
}

void CDInfoModel::setCustomTrackData(const int tracknumber, const QString &type, const QVariant &data)
{
    beginResetModel();
    if (cd_info.metadata().track(tracknumber).setCustom(type, data))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

const QVariant CDInfoModel::customTrackData(const int tracknumber, const QString &type) const
{
    return cd_info.metadata().track(tracknumber).getCustom(type);
}

void CDInfoModel::setCover(const QImage &cover)
{
    beginResetModel();
    if (cd_info.metadata().set(Metadata::Cover, cover))
        Q_EMIT metadataChanged(cd_info.metadata());
    endResetModel();
}

const QImage CDInfoModel::cover() const
{
    return cd_info.metadata().get(Metadata::Cover).value<QImage>();
}

bool CDInfoModel::loadCoverFromFile(const QString &filename)
{
    beginResetModel();
    bool result = cd_info.metadata().loadCoverFromFile(filename);
    endResetModel();
    if (cd_info.metadata().isModified())
        Q_EMIT metadataChanged(cd_info.metadata());
    return result;
}

bool CDInfoModel::saveCoverToFile(const QString &filename)
{
    return cd_info.metadata().saveCoverToFile(filename);
}

bool CDInfoModel::coverExists() const
{
    return cd_info.metadata().contains(Metadata::Cover);
}

void CDInfoModel::removeCover()
{
    cd_info.metadata().remove(Metadata::Cover);
    if (cd_info.metadata().isModified())
        Q_EMIT metadataChanged(cd_info.metadata());
}

void CDInfoModel::swapArtistAndTitleOfTracks()
{
    beginResetModel();
    cd_info.metadata().swapArtistAndTitleOfTracks();
    endResetModel();
    if (cd_info.metadata().isModified())
        Q_EMIT metadataChanged(cd_info.metadata());
}

void CDInfoModel::swapArtistAndAlbum()
{
    beginResetModel();
    cd_info.metadata().swapArtistAndAlbum();
    endResetModel();
    if (cd_info.metadata().isModified())
        Q_EMIT metadataChanged(cd_info.metadata());
}

void CDInfoModel::splitTitleOfTracks(const QString &divider)
{
    beginResetModel();
    cd_info.metadata().splitTitleOfTracks(divider);
    endResetModel();
    if (cd_info.metadata().isModified())
        Q_EMIT metadataChanged(cd_info.metadata());
}

void CDInfoModel::capitalizeTracks()
{
    beginResetModel();
    cd_info.metadata().capitalizeTracks();
    endResetModel();
    if (cd_info.metadata().isModified())
        Q_EMIT metadataChanged(cd_info.metadata());
}

void CDInfoModel::capitalizeHeader()
{
    beginResetModel();
    cd_info.metadata().capitalizeHeader();
    endResetModel();
    if (cd_info.metadata().isModified())
        Q_EMIT metadataChanged(cd_info.metadata());
}

void CDInfoModel::setTitleArtistsFromHeader()
{
    beginResetModel();
    cd_info.metadata().setTitleArtistsFromHeader();
    endResetModel();
    if (cd_info.metadata().isModified())
        Q_EMIT metadataChanged(cd_info.metadata());
}

void CDInfoModel::toggle(int row)
{
    p_toggle(row + 1);

    Q_EMIT hasSelection(0 != sel_tracks.count());
    Q_EMIT selectionChanged(sel_tracks.count());
}

bool CDInfoModel::isTrackInSelection(int tracknumber) const
{
    return sel_tracks.contains(tracknumber);
}

void CDInfoModel::invertSelection()
{
    for (int t = 1; t <= cd_info.toc().trackCount(); ++t)
        if (isValidAudioTrack(t))
            p_toggle(t);
    Q_EMIT hasSelection(0 != sel_tracks.count());
    Q_EMIT selectionChanged(sel_tracks.count());
}

void CDInfoModel::selectAll()
{
    sel_tracks.clear();
    invertSelection();
}

void CDInfoModel::selectNone()
{
    sel_tracks.clear();
    Q_EMIT hasSelection(false);
    Q_EMIT selectionChanged(0);
}

bool CDInfoModel::isModified() const
{
    return cd_info.metadata().isModified();
}

void CDInfoModel::confirm()
{
    cd_info.metadata().confirm();
}

void CDInfoModel::update(const CDInfo &cdInfo)
{
    beginResetModel();
    this->cd_info = cdInfo;
    setVarious(guessVarious());
    endResetModel();
    confirm();

    sel_tracks.clear();
    for (int t = 1; t <= cd_info.toc().trackCount(); ++t)
        if (isValidAudioTrack(t))
            sel_tracks.insert(t);

    Q_EMIT hasSelection(0 != sel_tracks.size());
    Q_EMIT selectionChanged(sel_tracks.count());
}

void CDInfoModel::clear()
{
    beginResetModel();
    cd_info.clear();
    sel_tracks.clear();
    endResetModel();
}

void CDInfoModel::p_toggle(const int tracknumber)
{
    if (sel_tracks.contains(tracknumber))
        sel_tracks.remove(tracknumber);
    else
        sel_tracks.insert(tracknumber);
}
}
