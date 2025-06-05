/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddamodel.h"

namespace Audex
{

CDDAModel::CDDAModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    Q_UNUSED(parent);
}

CDDAModel::~CDDAModel()
{
}

int CDDAModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (cdda.HTOAAvailable())
        return cdda.toc().trackCount() + 1;
    else
        return cdda.toc().trackCount();
}

int CDDAModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return COLUMN_COUNT;
}

QVariant CDDAModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() < 0 || index.row() >= rowCount())
        return QVariant();

    int track_index = index.row() + 1;
    if (cdda.HTOAAvailable())
        track_index--;

    if (role == Qt::TextAlignmentRole)
        return int(Qt::AlignLeft | Qt::AlignVCenter);

    if (role == Qt::ForegroundRole) {
        if (track_index == 0)
            return QVariant(QColor(Qt::darkRed));
        if (!cdda.toc().isAudioTrack(track_index))
            return QVariant(QColor(Qt::gray));
    }

    if ((role == Qt::DisplayRole) || role == Qt::EditRole) {
        switch (index.column()) {
        case COLUMN_TRACKNUMBER_INDEX: {
            int tracknumber_offset = cdda.metadata().get(Metadata::TrackNumberOffset, -1).toInt();
            if (tracknumber_offset == -1)
                return track_index;
            if (cdda.HTOAAvailable())
                return track_index + tracknumber_offset;
            else
                return track_index + tracknumber_offset - 1;
        }
        case COLUMN_LENGTH_INDEX: {
            if (track_index == 0)
                return QStringLiteral(u"%1:%2").arg(cdda.HTOALength() / 60, 2, 10, QChar(u'0')).arg(cdda.HTOALength() % 60, 2, 10, QChar(u'0'));
            if (!cdda.toc().isAudioTrack(track_index)) {
                return i18n("%1 MiB", QString::number(cdda.toc().sizeTrack(track_index), 'f', 2));
            }
            int length_of_track = 0;
            if (!cdda.toc().isEmpty())
                length_of_track = cdda.toc().lengthOfTrack(track_index);
            return QStringLiteral(u"%1:%2").arg(length_of_track / 60, 2, 10, QChar(u'0')).arg(length_of_track % 60, 2, 10, QChar(u'0'));
        }

        case COLUMN_ARTIST_INDEX:

            if (track_index == 0)
                return cdda.metadataHTOA().get(Metadata::Artist).toString();
            if (track_index <= cdda.metadata().trackCount())
                return cdda.metadata().track(track_index).get(Metadata::Artist).toString();
            break;

        case COLUMN_TITLE_INDEX:

            QString title;
            if (track_index == 0) {
                title = cdda.metadataHTOA().get(Metadata::Title).toString();
                if (!title.isEmpty())
                    return title;
                return i18n("Hidden track");
            }

            if (track_index <= cdda.metadata().trackCount())
                title = cdda.metadata().track(track_index).get(Metadata::Title).toString();

            if (title.isEmpty()) {
                if (!cdda.toc().isAudioTrack(track_index))
                    return i18n("Data Track %1", track_index);
                return i18n("Track %1", track_index);
            }

            return title;
        }
    }

    if (role == Qt::CheckStateRole && index.column() == COLUMN_RIP_INDEX) {
        return isTrackInSelection(track_index) ? Qt::Checked : Qt::Unchecked;
    }

    return QVariant();
}

bool CDDAModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (index.row() < 0 || index.row() >= rowCount())
        return false;

    int track_index = index.row() + 1;
    if (cdda.HTOAAvailable())
        track_index--;

    if (role == Qt::EditRole) {
        bool changed = false;
        switch (index.column()) {
        case COLUMN_ARTIST_INDEX:
            if (track_index == 0)
                changed = cdda.metadataHTOA().set(Metadata::Artist, value);
            else
                changed = cdda.metadata().track(track_index).set(Metadata::Artist, value);
            break;
        case COLUMN_TITLE_INDEX:
            if (track_index == 0)
                changed = cdda.metadataHTOA().set(Metadata::Title, value);
            else
                changed = cdda.metadata().track(track_index).set(Metadata::Title, value);
            break;
        default:
            return false;
        }
        if (changed) {
            Q_EMIT dataChanged(index, index);
            Q_EMIT metadataChanged(cdda.metadata());
        }
        return changed;
    }

    if (role == Qt::CheckStateRole && index.column() == COLUMN_RIP_INDEX) {
        if (cdda.HTOAAvailable() && index.row() == 0)
            toggle(index.row());
        else if (cdda.toc().isAudioTrack(track_index))
            toggle(track_index);
        return true;
    }

    return false;
}

QVariant CDDAModel::headerData(int section, Qt::Orientation orientation, int role) const
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
            default:;
            }
            break;
        case Qt::TextAlignmentRole:
            return Qt::AlignLeft;
        default:;
        }
    } else if (orientation == Qt::Vertical) {
        if (role == Qt::DisplayRole) {
            return QVariant(section + 1);
        }
    }

    return QVariant();
}

Qt::ItemFlags CDDAModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = Qt::ItemIsEnabled;
    if ((index.column() == COLUMN_ARTIST_INDEX) || (index.column() == COLUMN_TITLE_INDEX))
        flags |= Qt::ItemIsEditable;
    if (index.column() == COLUMN_RIP_INDEX)
        flags |= Qt::ItemIsUserCheckable;
    return flags;
}

void CDDAModel::setArtist(const QString &artist)
{
    beginResetModel();
    if (cdda.metadata().set(Metadata::Artist, artist))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

const QString CDDAModel::artist() const
{
    return cdda.metadata().get(Metadata::Artist).toString();
}

void CDDAModel::setAlbum(const QString &album)
{
    beginResetModel();
    if (cdda.metadata().set(Metadata::Album, album))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

const QString CDDAModel::album() const
{
    return cdda.metadata().get(Metadata::Album).toString();
}

void CDDAModel::setCDDBCategory(const QString &cat)
{
    beginResetModel();
    if (cdda.metadata().set(Metadata::CDDBCategory, cat))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

const QString CDDAModel::cddbCategory() const
{
    return cdda.metadata().get(Metadata::CDDBCategory).toString();
}

void CDDAModel::setGenre(const QString &genre)
{
    beginResetModel();
    if (cdda.metadata().set(Metadata::Genre, genre))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

const QString CDDAModel::genre() const
{
    return cdda.metadata().get(Metadata::Genre).toString();
}

void CDDAModel::setYear(const QString &year)
{
    beginResetModel();
    if (cdda.metadata().set(Metadata::Year, year))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

const QString CDDAModel::year() const
{
    return cdda.metadata().get(Metadata::Year).toString();
}

void CDDAModel::setComment(const QStringList &comment)
{
    beginResetModel();
    if (cdda.metadata().set(Metadata::Comment, comment))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

const QStringList CDDAModel::comment() const
{
    return cdda.metadata().get(Metadata::Comment).toStringList();
}

void CDDAModel::setDiscNumber(const int number)
{
    beginResetModel();
    if (cdda.metadata().set(Metadata::DiscNumber, number))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

int CDDAModel::discNumber() const
{
    if (!cdda.metadata().isMultiDisc())
        return 0;
    return cdda.metadata().get(Metadata::DiscNumber).toInt();
}

void CDDAModel::setDiscCount(const int count)
{
    beginResetModel();
    if (cdda.metadata().set(Metadata::DiscCount, count))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

int CDDAModel::discCount() const
{
    if (!cdda.metadata().isMultiDisc())
        return 0;
    return cdda.metadata().get(Metadata::DiscCount).toInt();
}

void CDDAModel::setTrackNumberOffset(const int offset)
{
    beginResetModel();
    if (offset == 0 && cdda.metadata().remove(Metadata::TrackNumberOffset))
        Q_EMIT metadataChanged(cdda.metadata());
    else if (cdda.metadata().set(Metadata::TrackNumberOffset, offset))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

int CDDAModel::guessMultiDisc(QString &newTitle) const
{
    return cdda.metadata().guessMultiDisc(newTitle);
}

void CDDAModel::setMultiDisc(const bool multi)
{
    beginResetModel();
    if (cdda.metadata().set(Metadata::MultiDisc, multi))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

bool CDDAModel::isMultiDisc() const
{
    return cdda.metadata().get(Metadata::MultiDisc).toBool();
}

void CDDAModel::setCustomData(const QString &type, const QVariant &data)
{
    beginResetModel();
    if (cdda.metadata().setCustom(type, data))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

bool CDDAModel::guessVarious() const
{
    return cdda.metadata().guessVarious();
}

void CDDAModel::setVarious(const bool various)
{
    if (cdda.metadata().set(Metadata::VariousArtists, various))
        Q_EMIT metadataChanged(cdda.metadata());
}

bool CDDAModel::isVarious() const
{
    return cdda.metadata().get(Metadata::VariousArtists).toBool();
}

const QVariant CDDAModel::customData(const QString &type) const
{
    return cdda.metadata().getCustom(type);
}

bool CDDAModel::isValidAudioTrack(const int tracknumber) const
{
    if (tracknumber < 1 && tracknumber > cdda.toc().trackCount())
        return false;
    return cdda.toc().isAudioTrack(tracknumber);
}

void CDDAModel::setCustomTrackData(const int tracknumber, const QString &type, const QVariant &data)
{
    beginResetModel();
    if (cdda.metadata().track(tracknumber).setCustom(type, data))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

const QVariant CDDAModel::customTrackData(const int tracknumber, const QString &type) const
{
    return cdda.metadata().track(tracknumber).getCustom(type);
}

void CDDAModel::setCover(const QImage &cover)
{
    beginResetModel();
    if (cdda.metadata().set(Metadata::Cover, cover))
        Q_EMIT metadataChanged(cdda.metadata());
    endResetModel();
}

const QImage CDDAModel::cover() const
{
    return cdda.metadata().get(Metadata::Cover).value<QImage>();
}

bool CDDAModel::loadCoverFromFile(const QString &filename)
{
    beginResetModel();
    bool result = cdda.metadata().loadCoverFromFile(filename);
    endResetModel();
    if (cdda.metadata().isModified())
        Q_EMIT metadataChanged(cdda.metadata());
    return result;
}

bool CDDAModel::saveCoverToFile(const QString &filename)
{
    return cdda.metadata().saveCoverToFile(filename);
}

bool CDDAModel::coverExists() const
{
    return cdda.metadata().contains(Metadata::Cover);
}

void CDDAModel::removeCover()
{
    cdda.metadata().remove(Metadata::Cover);
    if (cdda.metadata().isModified())
        Q_EMIT metadataChanged(cdda.metadata());
}

void CDDAModel::swapArtistAndTitleOfTracks()
{
    beginResetModel();
    cdda.metadata().swapArtistAndTitleOfTracks();
    endResetModel();
    if (cdda.metadata().isModified())
        Q_EMIT metadataChanged(cdda.metadata());
}

void CDDAModel::swapArtistAndAlbum()
{
    beginResetModel();
    cdda.metadata().swapArtistAndAlbum();
    endResetModel();
    if (cdda.metadata().isModified())
        Q_EMIT metadataChanged(cdda.metadata());
}

void CDDAModel::splitTitleOfTracks(const QString &divider)
{
    beginResetModel();
    cdda.metadata().splitTitleOfTracks(divider);
    endResetModel();
    if (cdda.metadata().isModified())
        Q_EMIT metadataChanged(cdda.metadata());
}

void CDDAModel::capitalizeTracks()
{
    beginResetModel();
    cdda.metadata().capitalizeTracks();
    endResetModel();
    if (cdda.metadata().isModified())
        Q_EMIT metadataChanged(cdda.metadata());
}

void CDDAModel::capitalizeHeader()
{
    beginResetModel();
    cdda.metadata().capitalizeHeader();
    endResetModel();
    if (cdda.metadata().isModified())
        Q_EMIT metadataChanged(cdda.metadata());
}

void CDDAModel::setTitleArtistsFromHeader()
{
    beginResetModel();
    cdda.metadata().setTitleArtistsFromHeader();
    endResetModel();
    if (cdda.metadata().isModified())
        Q_EMIT metadataChanged(cdda.metadata());
}

void CDDAModel::toggle(int row)
{
    p_toggle(row);
    Q_EMIT hasSelection(0 != sel_tracks.count());
    Q_EMIT selectionChanged(sel_tracks.count());
}

bool CDDAModel::isTrackInSelection(int tracknumber) const
{
    return sel_tracks.contains(tracknumber);
}

void CDDAModel::invertSelection()
{
    for (int t = 0; t <= cdda.toc().trackCount(); ++t) {
        if (t == 0) {
            if (cdda.HTOAAvailable())
                p_toggle(t);
        } else if (isValidAudioTrack(t)) {
            p_toggle(t);
        }
    }
    Q_EMIT hasSelection(0 != sel_tracks.count());
    Q_EMIT selectionChanged(sel_tracks.count());
}

void CDDAModel::selectAll()
{
    sel_tracks.clear();
    invertSelection();
}

void CDDAModel::selectNone()
{
    sel_tracks.clear();
    Q_EMIT hasSelection(false);
    Q_EMIT selectionChanged(0);
}

bool CDDAModel::isModified() const
{
    return cdda.metadata().isModified();
}

void CDDAModel::confirm()
{
    cdda.metadata().confirm();
}

bool CDDAModel::hasMetadata() const
{
    return cdda.metadata().isEmpty();
}

void CDDAModel::update(const CDDA &cdda)
{
    beginResetModel();

    this->cdda = cdda;

    setVarious(guessVarious());

    if (!isVarious() && cdda.HTOAAvailable() && cdda.metadataHTOA().get(Audex::Metadata::Artist, QString()).toString().isEmpty()) {
        QString artist = cdda.metadata().get(Audex::Metadata::Artist, QString()).toString();
        if (!artist.isEmpty())
            this->cdda.metadataHTOA().set(Audex::Metadata::Artist, artist);
    }

    endResetModel();

    confirm();

    sel_tracks.clear();
    for (int t = 0; t <= cdda.toc().trackCount(); ++t) {
        if (t == 0) {
            if (cdda.HTOAAvailable())
                sel_tracks.insert(t);
        } else if (isValidAudioTrack(t)) {
            sel_tracks.insert(t);
        }
    }

    Q_EMIT hasSelection(0 != sel_tracks.size());
    Q_EMIT selectionChanged(sel_tracks.count());
}

void CDDAModel::clear()
{
    beginResetModel();
    cdda.clear();
    sel_tracks.clear();
    endResetModel();
}

void CDDAModel::p_toggle(const int tracknumber)
{
    if (sel_tracks.contains(tracknumber))
        sel_tracks.remove(tracknumber);
    else
        sel_tracks.insert(tracknumber);
}
}
