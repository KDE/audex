/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdinfomodel.h"

#include <QColor>
#include <QFont>
#include <QGuiApplication>
#include <QPalette>

using namespace Qt::StringLiterals;

namespace Audex
{

using Metadata::Field;

namespace
{

QString formatLength(int sectors)
{
    const int seconds = sectors / Cdda::SectorsPerSecond;
    return u"%1:%2"_s.arg(seconds / 60).arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

int columnForField(Field field)
{
    switch (field) {
    case Field::Artist:
        return CDInfoModel::ArtistColumn;
    case Field::Title:
        return CDInfoModel::TitleColumn;
    default:
        return -1;
    }
}

}

CDInfoModel::CDInfoModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

// ---- disc -------------------------------------------------------------------------

void CDInfoModel::setCDInfo(const CDInfo &info)
{
    beginResetModel();
    m_info = info;
    m_entries = m_info.entries();
    m_selected.clear();
    for (int number : m_info.audioTrackNumbers(true))
        m_selected.insert(number);
    m_coverImageValid = false;
    if (!m_info.isEmpty())
        m_info.completeMetadata();
    m_info.metadata().confirm();
    endResetModel();

    Q_EMIT albumChanged();
    Q_EMIT coverChanged();
    Q_EMIT metadataChanged();
    Q_EMIT selectionChanged(selectedCount());
}

void CDInfoModel::clear()
{
    setCDInfo(CDInfo());
}

int CDInfoModel::rowForTrack(int number) const
{
    for (int row = 0; row < m_entries.size(); ++row)
        if (m_entries.at(row).number == number)
            return row;
    return -1;
}

int CDInfoModel::trackForRow(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return -1;
    return m_entries.at(row).number;
}

// ---- item model ---------------------------------------------------------------------

int CDInfoModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

int CDInfoModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QString CDInfoModel::placeholderTitle(const DiscEntry &entry) const
{
    if (entry.hidden)
        return tr("Hidden track");
    if (!entry.audio)
        return tr("Data track %1").arg(entry.number);
    return tr("Track %1").arg(m_info.displayTrackNumber(entry.number));
}

QVariant CDInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !checkIndex(index, CheckIndexOption::ParentIsInvalid))
        return QVariant();

    const DiscEntry &entry = m_entries.at(index.row());
    const Metadata::Track &track = m_info.metadata().track(entry.number);
    const int column = index.column();
    const QString title = track.text(Field::Title);

    switch (role) {
    case TrackNumberRole:
        return entry.number;
    case IsAudioRole:
        return entry.audio;
    case IsHiddenRole:
        return entry.hidden;
    case FirstSectorRole:
        return entry.firstLba;
    case SectorCountRole:
        return entry.sectorCount();
    case IsPlaceholderRole:
        return title.isEmpty();

    case Qt::CheckStateRole:
        if (column == RipColumn && isSelectable(entry.number) && !m_selectionLocked)
            return isSelected(entry.number) ? Qt::Checked : Qt::Unchecked;
        return QVariant();

    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (column) {
        case TrackNumberColumn:
            return m_info.displayTrackNumber(entry.number);
        case ArtistColumn:
            return track.text(Field::Artist);
        case TitleColumn:
            if (role == Qt::DisplayRole && title.isEmpty())
                return placeholderTitle(entry);
            return title;
        case LengthColumn:
            if (role == Qt::EditRole)
                return entry.sectorCount();
            if (!entry.audio)
                return tr("%1 MiB").arg(double(entry.sectorCount()) * 2048.0 / (1024.0 * 1024.0), 0, 'f', 1);
            return formatLength(entry.sectorCount());
        default:
            return QVariant();
        }

    case Qt::ToolTipRole: {
        QStringList lines;
        if (entry.hidden)
            lines << tr("Hidden track one audio (before track 1)");
        if (!entry.audio)
            lines << tr("Data track (session %1), cannot be ripped").arg(entry.session);
        if (column == LengthColumn) {
            lines << tr("%1 sectors, LBA %2 – %3").arg(entry.sectorCount()).arg(entry.firstLba).arg(entry.lastLba);
        } else {
            const QList<std::pair<Field, QString>> details{
                {Field::Composer, tr("Composer")},
                {Field::Songwriter, tr("Songwriter")},
                {Field::ISRC, tr("ISRC")},
                {Field::Comment, tr("Comment")},
            };
            for (const auto &[field, label] : details)
                if (track.contains(field))
                    lines << tr("%1: %2").arg(label, track.text(field));
        }
        return lines.isEmpty() ? QVariant() : QVariant(lines.join(u'\n'));
    }

    case Qt::TextAlignmentRole:
        if (column == TrackNumberColumn || column == LengthColumn)
            return QVariant::fromValue(Qt::AlignRight | Qt::AlignVCenter);
        return QVariant::fromValue(Qt::AlignLeft | Qt::AlignVCenter);

    case Qt::FontRole:
        if (entry.hidden) {
            QFont font;
            font.setItalic(true);
            return font;
        }
        return QVariant();

    case Qt::ForegroundRole:
        if (column == TitleColumn && title.isEmpty()) {
            const QColor color = qGuiApp ? qGuiApp->palette().color(QPalette::PlaceholderText) : QColor(Qt::gray);
            return color;
        }
        return QVariant();
    }
    return QVariant();
}

bool CDInfoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || !checkIndex(index, CheckIndexOption::ParentIsInvalid))
        return false;
    const DiscEntry &entry = m_entries.at(index.row());

    if (role == Qt::CheckStateRole && index.column() == RipColumn) {
        if (!isSelectable(entry.number) || m_selectionLocked)
            return false;
        setSelected(entry.number, value.value<Qt::CheckState>() == Qt::Checked);
        return true;
    }

    if (role != Qt::EditRole || !entry.audio)
        return false;
    if (index.column() == ArtistColumn) {
        setTrackValue(entry.number, Field::Artist, value.toString());
        return true;
    }
    if (index.column() == TitleColumn) {
        setTrackValue(entry.number, Field::Title, value.toString());
        return true;
    }
    return false;
}

QVariant CDInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();
    if (role == Qt::DisplayRole) {
        switch (section) {
        case RipColumn:
            return tr("Rip");
        case TrackNumberColumn:
            return tr("#");
        case ArtistColumn:
            return tr("Artist");
        case TitleColumn:
            return tr("Title");
        case LengthColumn:
            return tr("Length");
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (section == TrackNumberColumn || section == LengthColumn)
            return QVariant::fromValue(Qt::AlignRight | Qt::AlignVCenter);
        return QVariant::fromValue(Qt::AlignLeft | Qt::AlignVCenter);
    }
    return QVariant();
}

Qt::ItemFlags CDInfoModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || !checkIndex(index, CheckIndexOption::ParentIsInvalid))
        return Qt::NoItemFlags;
    const DiscEntry &entry = m_entries.at(index.row());
    Qt::ItemFlags f = Qt::ItemIsSelectable;
    if (!entry.audio)
        return f; // shown disabled
    f |= Qt::ItemIsEnabled;
    // NOTE: no Qt::ItemIsUserCheckable on RipColumn. The checkbox is display
    // only (CheckStateRole); toggling happens exclusively via the view's
    // clicked() signal in MainWindow. Otherwise a click on the indicator
    // would toggle twice (once by the view via setData, once by the slot).
    if (index.column() == ArtistColumn || index.column() == TitleColumn)
        f |= Qt::ItemIsEditable;
    return f;
}

QHash<int, QByteArray> CDInfoModel::roleNames() const
{
    QHash<int, QByteArray> names = QAbstractTableModel::roleNames();
    names.insert(TrackNumberRole, "trackNumber");
    names.insert(IsAudioRole, "isAudio");
    names.insert(IsHiddenRole, "isHidden");
    names.insert(FirstSectorRole, "firstSector");
    names.insert(SectorCountRole, "sectorCount");
    names.insert(IsPlaceholderRole, "isPlaceholder");
    return names;
}

// ---- album values -------------------------------------------------------------------

QVariant CDInfoModel::albumValue(Field field) const
{
    return m_info.metadata().value(field);
}

bool CDInfoModel::setAlbumValue(Field field, const QVariant &value)
{
    if (!m_info.metadata().setValue(field, value))
        return false;
    if (field == Field::TrackNumberOffset)
        emitTrackColumnsChanged(TrackNumberColumn, TitleColumn); // numbers and placeholders
    Q_EMIT albumChanged();
    Q_EMIT metadataChanged();
    return true;
}

QString CDInfoModel::artist() const
{
    return m_info.metadata().text(Field::Artist);
}
void CDInfoModel::setArtist(const QString &value)
{
    setAlbumValue(Field::Artist, value);
}
QString CDInfoModel::album() const
{
    return m_info.metadata().text(Field::Album);
}
void CDInfoModel::setAlbum(const QString &value)
{
    setAlbumValue(Field::Album, value);
}
QString CDInfoModel::year() const
{
    return m_info.metadata().text(Field::Year);
}
void CDInfoModel::setYear(const QString &value)
{
    setAlbumValue(Field::Year, value);
}
QString CDInfoModel::genre() const
{
    return m_info.metadata().text(Field::Genre);
}
void CDInfoModel::setGenre(const QString &value)
{
    setAlbumValue(Field::Genre, value);
}
QString CDInfoModel::comment() const
{
    return m_info.metadata().text(Field::Comment);
}
void CDInfoModel::setComment(const QString &value)
{
    setAlbumValue(Field::Comment, value);
}
int CDInfoModel::discNumber() const
{
    return m_info.metadata().number(Field::DiscNumber);
}
void CDInfoModel::setDiscNumber(int value)
{
    setAlbumValue(Field::DiscNumber, value);
}
int CDInfoModel::discCount() const
{
    return m_info.metadata().number(Field::DiscCount);
}
void CDInfoModel::setDiscCount(int value)
{
    setAlbumValue(Field::DiscCount, value);
}
int CDInfoModel::trackNumberOffset() const
{
    return m_info.metadata().number(Field::TrackNumberOffset);
}
void CDInfoModel::setTrackNumberOffset(int value)
{
    setAlbumValue(Field::TrackNumberOffset, value);
}
bool CDInfoModel::variousArtists() const
{
    return m_info.metadata().flag(Field::VariousArtists);
}
void CDInfoModel::setVariousArtists(bool value)
{
    setAlbumValue(Field::VariousArtists, value);
}

// ---- track values -------------------------------------------------------------------

QVariant CDInfoModel::trackValue(int number, Field field) const
{
    return m_info.metadata().track(number).value(field);
}

bool CDInfoModel::setTrackValue(int number, Field field, const QVariant &value)
{
    const int row = rowForTrack(number);
    if (row < 0)
        return false;
    if (!m_info.trackMetadata(number).setValue(field, value))
        return false;
    const int column = columnForField(field);
    if (column >= 0)
        Q_EMIT dataChanged(index(row, column), index(row, column));
    else
        Q_EMIT dataChanged(index(row, 0), index(row, ColumnCount - 1), {Qt::ToolTipRole});
    Q_EMIT metadataChanged();
    return true;
}

// ---- cover ---------------------------------------------------------------------------

Metadata::CoverArt CDInfoModel::cover() const
{
    return m_info.metadata().cover();
}

QImage CDInfoModel::coverImage() const
{
    if (!m_coverImageValid) {
        const Metadata::CoverArt &c = m_info.metadata().cover();
        m_coverImage = c.isNull() ? QImage() : QImage::fromData(c.data);
        m_coverImageValid = true;
    }
    return m_coverImage;
}

bool CDInfoModel::setCover(const Metadata::CoverArt &cover)
{
    if (!m_info.metadata().setCover(cover))
        return false;
    m_coverImageValid = false;
    Q_EMIT coverChanged();
    Q_EMIT metadataChanged();
    return true;
}

bool CDInfoModel::loadCoverFromFile(const QString &path, QString *error)
{
    const auto cover = Metadata::CoverArt::fromFile(path, error);
    if (!cover)
        return false;
    setCover(*cover);
    return true;
}

bool CDInfoModel::saveCoverToFile(const QString &path, QString *error) const
{
    return m_info.metadata().cover().saveToFile(path, error);
}

bool CDInfoModel::removeCover()
{
    return setCover(Metadata::CoverArt());
}

// ---- bulk edits -----------------------------------------------------------------------

void CDInfoModel::emitTrackColumnsChanged(int firstColumn, int lastColumn)
{
    if (m_entries.isEmpty())
        return;
    Q_EMIT dataChanged(index(0, firstColumn), index(int(m_entries.size()) - 1, lastColumn));
}

void CDInfoModel::afterBulkEdit(bool albumMayHaveChanged)
{
    emitTrackColumnsChanged(TrackNumberColumn, TitleColumn);
    if (albumMayHaveChanged)
        Q_EMIT albumChanged();
    Q_EMIT metadataChanged();
}

void CDInfoModel::applyCandidate(const MetadataCandidate &candidate, Metadata::MergePolicy policy)
{
    const Metadata::CoverArt coverBefore = m_info.metadata().cover();
    m_info.applyCandidate(candidate, policy);
    emitTrackColumnsChanged(0, ColumnCount - 1);
    Q_EMIT albumChanged();
    if (!(coverBefore == m_info.metadata().cover())) {
        m_coverImageValid = false;
        Q_EMIT coverChanged();
    }
    Q_EMIT metadataChanged();
}

void CDInfoModel::swapTrackArtistsAndTitles()
{
    if (m_info.metadata().swapTrackArtistsAndTitles())
        afterBulkEdit(false);
}

void CDInfoModel::swapArtistAndAlbum()
{
    if (m_info.metadata().swapArtistAndAlbum()) {
        Q_EMIT albumChanged();
        Q_EMIT metadataChanged();
    }
}

void CDInfoModel::splitTrackTitles(const QString &divider)
{
    if (m_info.metadata().splitTrackTitles(divider))
        afterBulkEdit(false);
}

void CDInfoModel::capitalizeTracks()
{
    if (m_info.metadata().capitalizeTracks())
        afterBulkEdit(false);
}

void CDInfoModel::capitalizeAlbum()
{
    if (m_info.metadata().capitalizeAlbum()) {
        Q_EMIT albumChanged();
        Q_EMIT metadataChanged();
    }
}

void CDInfoModel::setTrackArtistsFromAlbum(bool onlyEmpty)
{
    // make sure every audio entry has a track entry to receive the artist
    for (int number : m_info.audioTrackNumbers(true))
        m_info.trackMetadata(number);
    if (m_info.metadata().setTrackArtistsFromAlbum(onlyEmpty))
        afterBulkEdit(false);
}

// ---- selection ----------------------------------------------------------------------------

void CDInfoModel::setSelectionLocked(bool locked)
{
    if (locked == m_selectionLocked)
        return;
    m_selectionLocked = locked;
    if (!m_entries.isEmpty())
        Q_EMIT dataChanged(index(0, RipColumn), index(int(m_entries.size()) - 1, RipColumn), {Qt::CheckStateRole});
}

bool CDInfoModel::isSelectable(int number) const
{
    return m_info.isAudioTrack(number);
}

QList<int> CDInfoModel::selectedTracks() const
{
    QList<int> result(m_selected.cbegin(), m_selected.cend());
    std::sort(result.begin(), result.end());
    return result;
}

bool CDInfoModel::isSelected(int number) const
{
    return m_selected.contains(number);
}

void CDInfoModel::emitSelectionChanged(const QSet<int> &before)
{
    if (before == m_selected)
        return;
    const QSet<int> changed = (before - m_selected) + (m_selected - before);
    for (int number : changed) {
        const int row = rowForTrack(number);
        if (row >= 0)
            Q_EMIT dataChanged(index(row, RipColumn), index(row, RipColumn), {Qt::CheckStateRole});
    }
    Q_EMIT selectionChanged(selectedCount());
}

void CDInfoModel::setSelected(int number, bool selected)
{
    if (!isSelectable(number))
        return;
    const QSet<int> before = m_selected;
    if (selected)
        m_selected.insert(number);
    else
        m_selected.remove(number);
    emitSelectionChanged(before);
}

void CDInfoModel::selectAll()
{
    const QSet<int> before = m_selected;
    m_selected.clear();
    for (int number : m_info.audioTrackNumbers(true))
        m_selected.insert(number);
    emitSelectionChanged(before);
}

void CDInfoModel::selectNone()
{
    const QSet<int> before = m_selected;
    m_selected.clear();
    emitSelectionChanged(before);
}

void CDInfoModel::invertSelection()
{
    const QSet<int> before = m_selected;
    QSet<int> inverted;
    for (int number : m_info.audioTrackNumbers(true))
        if (!before.contains(number))
            inverted.insert(number);
    m_selected = inverted;
    emitSelectionChanged(before);
}

// ---- modification state ------------------------------------------------------------------

bool CDInfoModel::isModified() const
{
    return m_info.metadata().isModified();
}

void CDInfoModel::confirm()
{
    m_info.metadata().confirm();
}

}
