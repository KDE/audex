/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profilemodel.h"

// #include "utils/schemeparser.h"

#include <KLocalizedString>
#include <KSharedConfig>

#include <QFile>
#include <QFileInfo>
#include <QIcon>

ProfileModel::ProfileModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    revert();
}

ProfileModel::~ProfileModel()
{
    clear();
}

int ProfileModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return p_cache.count();
}

int ProfileModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return PROFILE_MODEL_COLUMN_NUM;
}

QVariant ProfileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if ((index.row() < 0) || (index.row() >= p_cache.count()))
        return QVariant();

    if (role == Qt::TextAlignmentRole)
        return int(Qt::AlignLeft | Qt::AlignVCenter);

    if ((role == Qt::DisplayRole) || (role == Qt::EditRole)) {
        switch (index.column()) {
        case PROFILE_MODEL_COLUMN_PROFILEINDEX_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_PROFILEINDEX_KEY];
        case PROFILE_MODEL_COLUMN_NAME_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_NAME_KEY];
        case PROFILE_MODEL_COLUMN_ICON_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_ICON_KEY];

        case PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_ENCODER_SELECTED_KEY];

        case PROFILE_MODEL_COLUMN_SCHEME_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_SCHEME_KEY];
        case PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_FAT32COMPATIBLE_KEY];
        case PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_UNDERSCORE_KEY];
        case PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_2DIGITSTRACKNUM_KEY];
        case PROFILE_MODEL_COLUMN_SC_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_SC_KEY];
        case PROFILE_MODEL_COLUMN_SC_SCALE_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_SC_SCALE_KEY];
        case PROFILE_MODEL_COLUMN_SC_SIZE_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_SC_SIZE_KEY];
        case PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_SC_FORMAT_KEY];
        case PROFILE_MODEL_COLUMN_SC_NAME_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_SC_NAME_KEY];
        case PROFILE_MODEL_COLUMN_PL_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_PL_KEY];
        case PROFILE_MODEL_COLUMN_PL_NAME_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_PL_NAME_KEY];
        case PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_PL_ABS_FILE_PATH_KEY];
        case PROFILE_MODEL_COLUMN_PL_UTF8_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_PL_UTF8_KEY];
        case PROFILE_MODEL_COLUMN_LOG_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_LOG_KEY];
        case PROFILE_MODEL_COLUMN_LOG_NAME_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_LOG_NAME_KEY];
        case PROFILE_MODEL_COLUMN_HOOK_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_HOOK_KEY];
        case PROFILE_MODEL_COLUMN_HOOK_COMMAND_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_HOOK_COMMAND_KEY];
        case PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY];
        case PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_KEY];
        case PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY];
        case PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY];
        case PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY];
        case PROFILE_MODEL_COLUMN_OUTPUT_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_OUTPUT_KEY];
        case PROFILE_MODEL_COLUMN_IMAGE_SCHEME_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_IMAGE_SCHEME_KEY];
        case PROFILE_MODEL_COLUMN_CUE_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_CUE_KEY];
        case PROFILE_MODEL_COLUMN_CUE_NAME_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_CUE_NAME_KEY];
        case PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_CUE_MCN_ISRC_KEY];
        case PROFILE_MODEL_COLUMN_CTDB_REPAIR_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_CTDB_REPAIR_KEY];
        case PROFILE_MODEL_COLUMN_CTDB_REPAIR_KEEP_ORIGINAL_INDEX:
            return p_cache.at(index.row())[PROFILE_MODEL_CTDB_REPAIR_KEEP_ORIGINAL_KEY];
        }
    }

    if (role == Qt::ToolTipRole) {
        const QString reason = unavailableReason(index.row());
        if (!reason.isEmpty())
            return reason;
        const auto encoder = static_cast<EncoderAssistant::Encoder>(p_cache.at(index.row())[PROFILE_MODEL_ENCODER_SELECTED_KEY].toInt());
        return isImage(index.row()) ? i18n("%1: the whole disc as one image file", EncoderAssistant::name(encoder))
                                    : i18n("%1: one file per track", EncoderAssistant::name(encoder));
    }

    if (role == Qt::DecorationRole) {
        if (!isAvailable(index.row()))
            return QIcon::fromTheme(QStringLiteral("dialog-warning"));

        QString iconName(p_cache.at(index.row())[PROFILE_MODEL_ICON_KEY].toString());

        if (!iconName.isEmpty()) {
            QIcon icon = QIcon::fromTheme(iconName);
            if (icon.isNull() && QFile::exists(iconName))
                icon = QIcon(iconName);
            if (!icon.isNull())
                return icon;
        }

        return QIcon::fromTheme(DEFAULT_ICON);
    }

    return QVariant();
}

bool ProfileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if ((index.row() < 0) || (index.row() >= p_cache.count()))
        return false;

    if (role == Qt::EditRole) {
        if (!validateData(index, value))
            return false;

        switch (index.column()) {
        case PROFILE_MODEL_COLUMN_PROFILEINDEX_INDEX:
            p_cache[index.row()][PROFILE_MODEL_PROFILEINDEX_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_NAME_INDEX:
            p_cache[index.row()][PROFILE_MODEL_NAME_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_ICON_INDEX:
            p_cache[index.row()][PROFILE_MODEL_ICON_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX:
            p_cache[index.row()][PROFILE_MODEL_ENCODER_SELECTED_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_SCHEME_INDEX:
            p_cache[index.row()][PROFILE_MODEL_SCHEME_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX:
            p_cache[index.row()][PROFILE_MODEL_FAT32COMPATIBLE_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX:
            p_cache[index.row()][PROFILE_MODEL_UNDERSCORE_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX:
            p_cache[index.row()][PROFILE_MODEL_2DIGITSTRACKNUM_KEY] = value;
            break;

        case PROFILE_MODEL_COLUMN_SC_INDEX:
            p_cache[index.row()][PROFILE_MODEL_SC_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_SC_SCALE_INDEX:
            p_cache[index.row()][PROFILE_MODEL_SC_SCALE_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_SC_SIZE_INDEX:
            p_cache[index.row()][PROFILE_MODEL_SC_SIZE_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX:
            p_cache[index.row()][PROFILE_MODEL_SC_FORMAT_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_SC_NAME_INDEX:
            p_cache[index.row()][PROFILE_MODEL_SC_NAME_KEY] = value;
            break;

        case PROFILE_MODEL_COLUMN_PL_INDEX:
            p_cache[index.row()][PROFILE_MODEL_PL_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_PL_NAME_INDEX:
            p_cache[index.row()][PROFILE_MODEL_PL_NAME_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX:
            p_cache[index.row()][PROFILE_MODEL_PL_ABS_FILE_PATH_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_PL_UTF8_INDEX:
            p_cache[index.row()][PROFILE_MODEL_PL_UTF8_KEY] = value;
            break;

        case PROFILE_MODEL_COLUMN_LOG_INDEX:
            p_cache[index.row()][PROFILE_MODEL_LOG_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_LOG_NAME_INDEX:
            p_cache[index.row()][PROFILE_MODEL_LOG_NAME_KEY] = value;
            break;

        case PROFILE_MODEL_COLUMN_HOOK_INDEX:
            p_cache[index.row()][PROFILE_MODEL_HOOK_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_HOOK_COMMAND_INDEX:
            p_cache[index.row()][PROFILE_MODEL_HOOK_COMMAND_KEY] = value;
            break;

        case PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_INDEX:
            p_cache[index.row()][PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_INDEX:
            p_cache[index.row()][PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX:
            p_cache[index.row()][PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX:
            p_cache[index.row()][PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX:
            p_cache[index.row()][PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY] = value;
            break;

        case PROFILE_MODEL_COLUMN_OUTPUT_INDEX:
            p_cache[index.row()][PROFILE_MODEL_OUTPUT_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_IMAGE_SCHEME_INDEX:
            p_cache[index.row()][PROFILE_MODEL_IMAGE_SCHEME_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_CUE_INDEX:
            p_cache[index.row()][PROFILE_MODEL_CUE_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_CUE_NAME_INDEX:
            p_cache[index.row()][PROFILE_MODEL_CUE_NAME_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX:
            p_cache[index.row()][PROFILE_MODEL_CUE_MCN_ISRC_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_CTDB_REPAIR_INDEX:
            p_cache[index.row()][PROFILE_MODEL_CTDB_REPAIR_KEY] = value;
            break;
        case PROFILE_MODEL_COLUMN_CTDB_REPAIR_KEEP_ORIGINAL_INDEX:
            p_cache[index.row()][PROFILE_MODEL_CTDB_REPAIR_KEEP_ORIGINAL_KEY] = value;
            break;

        default:
            break;
        }

        // Notify attached views about the changed cell instead of resetting
        // the whole model (a reset drops selection, current index and scroll
        // position).
        Q_EMIT dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});

        // The icon is delivered as Qt::DecorationRole for every column of the
        // row, so an icon change affects the complete row.
        if (index.column() == PROFILE_MODEL_COLUMN_ICON_INDEX)
            Q_EMIT dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(PROFILE_MODEL_COLUMN_NUM - 1), {Qt::DecorationRole});

        // availability (icon, tooltip, enabled state) and the tooltip
        // follow the encoder and the output type
        if (index.column() == PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX || index.column() == PROFILE_MODEL_COLUMN_OUTPUT_INDEX)
            Q_EMIT dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(PROFILE_MODEL_COLUMN_NUM - 1), {Qt::DecorationRole, Qt::ToolTipRole});

        return true;
    }

    p_error = Error(i18n("Unknown error. No index found in profile model."), i18n("This is an internal error. Please report."), Error::ERROR, this);

    return false;
}

bool ProfileModel::removeRows(int row, int count, const QModelIndex &parent)
{
    // flat table model: rows only exist below the root
    if (parent.isValid())
        return false;

    if ((row < 0) || (row >= p_cache.count()))
        return false;

    if (count <= 0)
        return false;

    // clamp the range to the end of the list
    const int n = qMin(count, int(p_cache.count()) - row);

    beginRemoveRows(parent, row, row + n - 1);
    p_cache.remove(row, n);
    endRemoveRows();

    // update current profile index. maybe current has been deleted?
    setCurrentProfileIndex(p_current_profile_index);

    Q_EMIT profilesRemovedOrInserted();

    return true;
}

bool ProfileModel::insertRows(int row, int count, const QModelIndex &parent)
{
    // flat table model: rows only exist below the root
    if (parent.isValid())
        return false;

    if ((row < 0) || (row > p_cache.count()))
        return false;

    if (count <= 0)
        return false;

    const bool wasEmpty = p_cache.isEmpty();

    beginInsertRows(parent, row, row + count - 1);
    // Insert one by one: p_new_profile() derives a unique profile index
    // from the current cache content. Inserting at the end appends.
    for (int i = 0; i < count; ++i)
        p_cache.insert(row + i, p_new_profile());
    endInsertRows();

    if (wasEmpty) {
        // set first profile as current index
        setCurrentProfileIndex(p_cache.at(0)[PROFILE_MODEL_PROFILEINDEX_KEY].toInt());
    }

    Q_EMIT profilesRemovedOrInserted();

    return true;
}

bool ProfileModel::validateData(const QModelIndex &index, const QVariant &value)
{
    switch (index.column()) {
    case PROFILE_MODEL_COLUMN_ICON_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_NAME_INDEX: {
        if (value.toString().isEmpty()) {
            p_error = Error(i18n("Profile name must not be empty."), i18n("You have given no name for the profile. Please set one."), Error::ERROR, this);
            return false;
        }
        // check if name is unique
        bool found = false;
        for (int i = 0; i < p_cache.count(); ++i) {
            if (i == index.row())
                continue;
            if (value.toString() == p_cache.at(i)[PROFILE_MODEL_NAME_KEY].toString()) {
                found = true;
                break;
            }
        }
        if (found) {
            p_error = Error(i18n("Profile name already exists."),
                            i18n("Your profile name %1 already exists in the set of profiles. Please choose a unique one.", value.toString()),
                            Error::ERROR,
                            this);
            return false;
        }
    }

    break;

    case PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX:
        if (value.toInt() == -1) {
            p_error = Error(i18n("Profile encoder is not defined."), i18n("You have given no encoder for the profile. Please set one."), Error::ERROR, this);
            return false;
        }
        break;

    case PROFILE_MODEL_COLUMN_SCHEME_INDEX:
        if (value.toString().isEmpty()) {
            p_error = Error(i18n("Profile filename scheme is not defined."),
                            i18n("You have given no filename scheme for the profile. Please set one."),
                            Error::ERROR,
                            this);
            return false;
        }
        break;

    case PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_SC_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_SC_SCALE_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_SC_SIZE_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX:
        if ((value.toString() != "JPEG") && (value.toString() != "PNG")) {
            p_error = Error(i18n("The image file format is unknown."),
                            i18n("Your given image file format is unknown. Please choose one of these formats: JPEG or PNG."),
                            Error::ERROR,
                            this);
            return false;
        }
        break;

    case PROFILE_MODEL_COLUMN_SC_NAME_INDEX:
        if (value.toString().isEmpty()) {
            p_error = Error(i18n("Cover name must not be empty."), i18n("You have given no name for the cover. Please set one."), Error::ERROR, this);
            return false;
        }
        break;

    case PROFILE_MODEL_COLUMN_PL_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_PL_NAME_INDEX:
        if (value.toString().isEmpty()) {
            p_error = Error(i18n("Playlist name must not be empty."), i18n("You have given no name for the playlist. Please set one."), Error::ERROR, this);
            return false;
        }
        break;

    case PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_PL_UTF8_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_LOG_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_LOG_NAME_INDEX:
        if (value.toString().isEmpty()) {
            p_error = Error(i18n("Log filename name must not be empty."), i18n("You have given no name for the log file. Please set one."), Error::ERROR, this);
            return false;
        }
        break;

    case PROFILE_MODEL_COLUMN_HOOK_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_HOOK_COMMAND_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_OUTPUT_INDEX: {
        const int output = value.toInt();
        if (output != PROFILE_OUTPUT_TRACKS && output != PROFILE_OUTPUT_IMAGE) {
            p_error = Error(i18n("Unknown output type."), i18n("This is an internal error. Please report."), Error::ERROR, this);
            return false;
        }
        // set the encoder first, then the output type
        const auto encoder = static_cast<EncoderAssistant::Encoder>(p_cache.at(index.row())[PROFILE_MODEL_ENCODER_SELECTED_KEY].toInt());
        if (output == PROFILE_OUTPUT_IMAGE && !EncoderAssistant::lossless(encoder)) {
            p_error = Error(i18n("An image requires a lossless encoder."),
                            i18n("Please choose WAVE or FLAC as encoder, or rip one file per track."),
                            Error::ERROR,
                            this);
            return false;
        }
    } break;

    case PROFILE_MODEL_COLUMN_IMAGE_SCHEME_INDEX:
        if (value.toString().isEmpty()) {
            p_error = Error(i18n("Image filename scheme is not defined."),
                            i18n("You have given no filename scheme for the image file. Please set one."),
                            Error::ERROR,
                            this);
            return false;
        }
        break;

    case PROFILE_MODEL_COLUMN_CUE_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_CUE_NAME_INDEX:
        if (value.toString().isEmpty()) {
            p_error = Error(i18n("Cue sheet name must not be empty."), i18n("You have given no name for the cue sheet. Please set one."), Error::ERROR, this);
            return false;
        }
        break;

    case PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_CTDB_REPAIR_INDEX:
        break;

    case PROFILE_MODEL_COLUMN_CTDB_REPAIR_KEEP_ORIGINAL_INDEX:
        break;

    default:
        return false;
    }

    return true;
}

void ProfileModel::setCurrentProfileIndex(int profile_index)
{
    int pi = profile_index;
    if (p_cache.count() == 0) {
        pi = -1;
    } else if (!indexExists(profile_index)) {
        // set first profile as current index
        pi = p_cache.at(0)[PROFILE_MODEL_PROFILEINDEX_KEY].toInt();
    }
    if (pi != p_current_profile_index) {
        p_current_profile_index = pi;
        // Use the process-wide shared config instance: a private KConfig
        // object would leave all other KSharedConfig users with stale data.
        KConfigGroup profilesGroup(KSharedConfig::openConfig(), QStringLiteral("Profiles"));
        profilesGroup.writeEntry("Standard", pi);
        profilesGroup.sync();
        Q_EMIT currentProfileIndexChanged(pi);
    }
}

int ProfileModel::setRowAsCurrentProfileIndex(int row)
{
    // Views and combo boxes report -1 when nothing is selected
    // (e.g. QComboBox::currentIndexChanged(-1) after clearing it).
    if ((row < 0) || (row >= p_cache.count()))
        return -1;

    const int i = p_cache.at(row).value(PROFILE_MODEL_PROFILEINDEX_KEY, -1).toInt();
    if (i != p_current_profile_index) {
        // an explicit choice replaces a profile remembered by ensureAvailableCurrentProfile()
        KConfigGroup fallbackGroup(KSharedConfig::openConfig(), QStringLiteral("ProfileFallback"));
        if (fallbackGroup.hasKey("Preferred")) {
            fallbackGroup.deleteEntry("Preferred");
            fallbackGroup.sync();
        }
    }
    setCurrentProfileIndex(i);
    return i;
}

int ProfileModel::currentProfileRow() const
{
    return getRowByIndex(p_current_profile_index);
}

int ProfileModel::getRowByIndex(int profile_index) const
{
    for (int i = 0; i < p_cache.count(); ++i)
        if (profile_index == p_cache.at(i)[PROFILE_MODEL_PROFILEINDEX_KEY].toInt())
            return i;
    return -1;
}

bool ProfileModel::isImage(int row) const
{
    if ((row < 0) || (row >= p_cache.count()))
        return false;
    return p_cache.at(row).value(PROFILE_MODEL_OUTPUT_KEY, DEFAULT_OUTPUT).toInt() == PROFILE_OUTPUT_IMAGE;
}

bool ProfileModel::isAvailable(int row) const
{
    if ((row < 0) || (row >= p_cache.count()))
        return false;
    return EncoderAssistant::available(static_cast<EncoderAssistant::Encoder>(p_cache.at(row)[PROFILE_MODEL_ENCODER_SELECTED_KEY].toInt()));
}

QString ProfileModel::unavailableReason(int row) const
{
    if ((row < 0) || (row >= p_cache.count()) || isAvailable(row))
        return QString();
    return EncoderAssistant::unavailableReason(static_cast<EncoderAssistant::Encoder>(p_cache.at(row)[PROFILE_MODEL_ENCODER_SELECTED_KEY].toInt()));
}

void ProfileModel::ensureAvailableCurrentProfile()
{
    KConfigGroup fallbackGroup(KSharedConfig::openConfig(), QStringLiteral("ProfileFallback"));

    // back to the profile that had to be replaced, once it is usable again
    const int preferred = fallbackGroup.readEntry("Preferred", -1);
    if (preferred != -1) {
        const int row = getRowByIndex(preferred);
        if (row == -1 || isAvailable(row)) {
            fallbackGroup.deleteEntry("Preferred");
            fallbackGroup.sync();
            if (row != -1) {
                setCurrentProfileIndex(preferred);
                return;
            }
        }
    }

    const int current = currentProfileRow();
    if (current == -1 || isAvailable(current))
        return;

    for (int row = 0; row < p_cache.count(); ++row) {
        if (!isAvailable(row))
            continue;
        if (!fallbackGroup.hasKey("Preferred")) {
            fallbackGroup.writeEntry("Preferred", p_current_profile_index);
            fallbackGroup.sync();
        }
        setCurrentProfileIndex(p_cache.at(row)[PROFILE_MODEL_PROFILEINDEX_KEY].toInt());
        return;
    }
}

void ProfileModel::clear()
{
    p_cache.clear();
    p_current_profile_index = -1;
}

bool ProfileModel::nameExists(const QString &name) const
{
    for (int j = 0; j < p_cache.count(); ++j)
        if (name == p_cache.at(j)[PROFILE_MODEL_NAME_KEY].toString())
            return true;

    return false;
}

bool ProfileModel::indexExists(int profile_index) const
{
    for (int j = 0; j < p_cache.count(); ++j)
        if (profile_index == p_cache.at(j)[PROFILE_MODEL_PROFILEINDEX_KEY].toInt())
            return true;

    return false;
}

int ProfileModel::getNewIndex() const
{
    QSet<int> indexes;
    QList<Profile>::ConstIterator it(p_cache.begin()), end(p_cache.end());

    for (; it != end; ++it)
        indexes.insert((*it)[PROFILE_MODEL_PROFILEINDEX_KEY].toInt());

    for (int i = 0; i < INT_MAX; ++i)
        if (!indexes.contains(i))
            return i;

    return -1;
}

static bool lessThan(const Profile &p1, const Profile &p2)
{
    return (QString::localeAwareCompare(p1[PROFILE_MODEL_NAME_KEY].toString(), p2[PROFILE_MODEL_NAME_KEY].toString()) < 0);
}

void ProfileModel::sortItems()
{
    beginResetModel();
    std::sort(p_cache.begin(), p_cache.end(), lessThan);
    endResetModel();
    Q_EMIT profilesRemovedOrInserted();
}

void ProfileModel::autoCreate()
{
    const bool wasEmpty = p_cache.isEmpty();
    bool created = false;

    // Profiles for encoders without their plugin are created as well: they
    // are shown disabled with a hint and work once the plugin is installed.
    const QList<QPair<EncoderAssistant::Encoder, bool>> defaults = {
        {EncoderAssistant::LAME, false},
        {EncoderAssistant::OPUSENC, false},
        {EncoderAssistant::FLAC, false},
        {EncoderAssistant::WAVE, false},
        {EncoderAssistant::FLAC, true},
        {EncoderAssistant::WAVE, true},
    };
    for (const auto &[encoder, image] : defaults) {
        const Profile p = p_new_encoder_profile(encoder, image);
        if (nameExists(p[PROFILE_MODEL_NAME_KEY].toString()))
            continue;
        p_cache.append(p);
        created = true;
    }

    if (created) {
        sortItems();
        if (wasEmpty) {
            // the first usable profile becomes the current one
            for (int row = 0; row < p_cache.count(); ++row) {
                if (isAvailable(row)) {
                    setCurrentProfileIndex(p_cache.at(row)[PROFILE_MODEL_PROFILEINDEX_KEY].toInt());
                    break;
                }
            }
        }
    }

    commit();
}

EncoderAssistant::Encoder ProfileModel::getSelectedEncoderFromCurrentIndex()
{
    return (EncoderAssistant::Encoder)data(index(currentProfileRow(), PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX)).toInt();
}

const Parameters ProfileModel::getSelectedEncoderParametersFromCurrentIndex()
{
    Parameters parameters;

    EncoderAssistant::Encoder encoder = getSelectedEncoderFromCurrentIndex();
    // what parameters does the encoder start with?
    switch (encoder) {
    case EncoderAssistant::LAME:
        parameters.fromString(data(index(currentProfileRow(), PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_INDEX)).toString());
        break;
    case EncoderAssistant::OPUSENC:
        parameters.fromString(data(index(currentProfileRow(), PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_INDEX)).toString());
        break;
    case EncoderAssistant::FLAC:
        parameters.fromString(data(index(currentProfileRow(), PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX)).toString());
        break;
    case EncoderAssistant::WAVE:
        parameters.fromString(data(index(currentProfileRow(), PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX)).toString());
        break;
    case EncoderAssistant::CUSTOM:
        parameters.fromString(data(index(currentProfileRow(), PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX)).toString());
        break;
    case EncoderAssistant::NUM:;
    }

    return parameters;
}

const QString ProfileModel::getSelectedEncoderSuffixFromCurrentIndex()
{
    EncoderAssistant::Encoder encoder = getSelectedEncoderFromCurrentIndex();
    Parameters parameters(getSelectedEncoderParametersFromCurrentIndex());

    switch (encoder) {
    case EncoderAssistant::LAME:
        return parameters.value(ENCODER_LAME_SUFFIX_KEY, ENCODER_LAME_SUFFIX).toString();
    case EncoderAssistant::OPUSENC:
        return parameters.value(ENCODER_OPUSENC_SUFFIX_KEY, ENCODER_OPUSENC_SUFFIX).toString();
    case EncoderAssistant::FLAC:
        return parameters.value(ENCODER_FLAC_SUFFIX_KEY, ENCODER_FLAC_SUFFIX).toString();
    case EncoderAssistant::WAVE:
        return parameters.value(ENCODER_WAVE_SUFFIX_KEY, ENCODER_WAVE_SUFFIX).toString();
    case EncoderAssistant::CUSTOM:
        return parameters.value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX).toString();
    case EncoderAssistant::NUM:
        return "";
    }

    return "";
}

const QString ProfileModel::getSelectedEncoderNameAndVersion()
{
    const EncoderAssistant::Encoder encoder = getSelectedEncoderFromCurrentIndex();
    const QString name = EncoderAssistant::encoderName(encoder);
    const QString version = EncoderAssistant::version(encoder);
    if (version.isEmpty())
        return name;
    if (version.startsWith(name))
        return version; // plugin version already contains the codec name
    return QString("%1 %2").arg(name, version);
}

bool ProfileModel::isSelectedEncoderWithEmbedCover()
{
    EncoderAssistant::Encoder encoder = getSelectedEncoderFromCurrentIndex();
    Parameters parameters(getSelectedEncoderParametersFromCurrentIndex());

    switch (encoder) {
    case EncoderAssistant::LAME:
        return parameters.value(ENCODER_LAME_EMBED_COVER_KEY, ENCODER_LAME_EMBED_COVER).toBool();
    case EncoderAssistant::OPUSENC:
        return parameters.value(ENCODER_OPUSENC_EMBED_COVER_KEY, ENCODER_OPUSENC_EMBED_COVER).toBool();
    case EncoderAssistant::FLAC:
        return parameters.value(ENCODER_FLAC_EMBED_COVER_KEY, ENCODER_FLAC_EMBED_COVER).toBool();
    case EncoderAssistant::WAVE:
        return false;
    case EncoderAssistant::CUSTOM:
        return true;
    case EncoderAssistant::NUM:
        return false;
    }

    return false;
}

bool ProfileModel::isCurrentImage() const
{
    return isImage(currentProfileRow());
}

Error ProfileModel::lastError() const
{
    return p_error;
}

void ProfileModel::commit()
{
    p_save(*KSharedConfig::openConfig());
}

const Profile ProfileModel::p_new_profile()
{
    Profile p;

    p[PROFILE_MODEL_PROFILEINDEX_KEY] = getNewIndex();
    p[PROFILE_MODEL_NAME_KEY] = DEFAULT_NAME;

    p[PROFILE_MODEL_ENCODER_SELECTED_KEY] = DEFAULT_ENCODER_SELECTED;

    p[PROFILE_MODEL_SCHEME_KEY] = DEFAULT_SCHEME;

    p[PROFILE_MODEL_FAT32COMPATIBLE_KEY] = DEFAULT_FAT32;
    p[PROFILE_MODEL_UNDERSCORE_KEY] = DEFAULT_UNDERSCORE;
    p[PROFILE_MODEL_2DIGITSTRACKNUM_KEY] = DEFAULT_2DIGITSTRACKNUM;

    p[PROFILE_MODEL_SC_KEY] = DEFAULT_SC;
    p[PROFILE_MODEL_SC_SCALE_KEY] = DEFAULT_SC_SCALE;
    p[PROFILE_MODEL_SC_SIZE_KEY] = DEFAULT_SC_SIZE;
    p[PROFILE_MODEL_SC_FORMAT_KEY] = DEFAULT_SC_FORMAT;
    p[PROFILE_MODEL_SC_NAME_KEY] = DEFAULT_SC_NAME;
    p[PROFILE_MODEL_PL_KEY] = DEFAULT_PL;
    p[PROFILE_MODEL_PL_NAME_KEY] = DEFAULT_PL_NAME;
    p[PROFILE_MODEL_PL_ABS_FILE_PATH_KEY] = DEFAULT_PL_ABS_FILE_PATH;
    p[PROFILE_MODEL_PL_UTF8_KEY] = DEFAULT_PL_UTF8;
    p[PROFILE_MODEL_LOG_KEY] = DEFAULT_LOG;
    p[PROFILE_MODEL_LOG_NAME_KEY] = DEFAULT_LOG_NAME;
    p[PROFILE_MODEL_HOOK_KEY] = DEFAULT_HOOK;
    p[PROFILE_MODEL_HOOK_COMMAND_KEY] = DEFAULT_HOOK_COMMAND;

    p[PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY] = DEFAULT_ENCODER_PARAMETERS;
    p[PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_KEY] = DEFAULT_ENCODER_PARAMETERS;
    p[PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY] = DEFAULT_ENCODER_PARAMETERS;
    p[PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY] = DEFAULT_ENCODER_PARAMETERS;
    p[PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY] = DEFAULT_ENCODER_PARAMETERS;

    p[PROFILE_MODEL_OUTPUT_KEY] = DEFAULT_OUTPUT;
    p[PROFILE_MODEL_IMAGE_SCHEME_KEY] = DEFAULT_IMAGE_SCHEME;
    p[PROFILE_MODEL_CUE_KEY] = DEFAULT_CUE;
    p[PROFILE_MODEL_CUE_NAME_KEY] = DEFAULT_CUE_NAME;
    p[PROFILE_MODEL_CUE_MCN_ISRC_KEY] = DEFAULT_CUE_MCN_ISRC;
    p[PROFILE_MODEL_CTDB_REPAIR_KEY] = DEFAULT_CTDB_REPAIR;
    p[PROFILE_MODEL_CTDB_REPAIR_KEEP_ORIGINAL_KEY] = DEFAULT_CTDB_REPAIR_KEEP_ORIGINAL;

    return p;
}

Profile ProfileModel::p_new_encoder_profile(EncoderAssistant::Encoder encoder, bool image)
{
    Profile p = p_new_profile();

    p[PROFILE_MODEL_ENCODER_SELECTED_KEY] = (int)encoder;
    const QString parameters = EncoderAssistant::stdParameters(encoder).toString();
    switch (encoder) {
    case EncoderAssistant::LAME:
        p[PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY] = parameters;
        break;
    case EncoderAssistant::OPUSENC:
        p[PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_KEY] = parameters;
        break;
    case EncoderAssistant::FLAC:
        p[PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY] = parameters;
        break;
    case EncoderAssistant::WAVE:
        p[PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY] = parameters;
        break;
    case EncoderAssistant::CUSTOM:
        p[PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY] = parameters;
        break;
    case EncoderAssistant::NUM:
        break;
    }

    if (image) {
        p[PROFILE_MODEL_NAME_KEY] = i18nc("%1 is the encoder, e.g. FLAC", "%1 Image (CUE)", EncoderAssistant::encoderName(encoder));
        p[PROFILE_MODEL_ICON_KEY] = DEFAULT_IMAGE_ICON;
        p[PROFILE_MODEL_OUTPUT_KEY] = PROFILE_OUTPUT_IMAGE;
    } else {
        p[PROFILE_MODEL_NAME_KEY] = EncoderAssistant::name(encoder);
        p[PROFILE_MODEL_ICON_KEY] = EncoderAssistant::icon(encoder);
    }

    return p;
}

void ProfileModel::p_new_name(QString &name)
{
    for (int j = 0; j < p_cache.count(); ++j)
        if (name == p_cache.at(j)[PROFILE_MODEL_NAME_KEY].toString()) {
            name = QString("%1 (%2)").arg(name).arg(i18n("Copy"));
            p_new_name(name);
            return;
        }
}

void ProfileModel::revert()
{
    // The cache is replaced completely, so attached views have to be reset.
    beginResetModel();
    clear();
    const int version = p_load(*KSharedConfig::openConfig());
    const bool migrate = (version < PROFILE_MODEL_VERSION) && !p_cache.isEmpty();
    if (migrate)
        p_migrateImageSettings();
    endResetModel();

    if (migrate)
        commit(); // persist migrated profiles right away
}

int ProfileModel::copy(const int profileRow)
{
    beginResetModel();
    if ((profileRow < 0) || (profileRow >= rowCount())) {
        endResetModel();
        return -1;
    }

    int key = getNewIndex();
    Profile p = p_cache[profileRow];

    QString name = p_cache[profileRow][PROFILE_MODEL_NAME_KEY].toString();
    p_new_name(name);
    p[PROFILE_MODEL_NAME_KEY] = name;
    p[PROFILE_MODEL_PROFILEINDEX_KEY] = key;
    p_cache.append(p);

    endResetModel();
    Q_EMIT profilesRemovedOrInserted();

    return key;
}

bool ProfileModel::saveProfilesToFile(const QString &filename)
{
    // Export files are standalone: SimpleConfig skips kdeglobals and any
    // cascading. Relative paths are made absolute here, as KConfig would
    // resolve them against the config location instead of the current
    // working directory.
    KConfig config(QFileInfo(filename).absoluteFilePath(), KConfig::SimpleConfig);
    p_save(config);
    return true;
}

bool ProfileModel::loadProfilesFromFile(const QString &filename)
{
    // see saveProfilesToFile() for why the path is made absolute
    const QString path = QFileInfo(filename).absoluteFilePath();

    // Validate before touching the model: p_load() clears the cache, and
    // commit() would persist the empty profile list afterwards.
    if (!QFile::exists(path)) {
        p_error = Error(i18n("Profile file not found."), i18n("The file %1 does not exist.", path), Error::ERROR, this);
        return false;
    }

    KConfig config(path, KConfig::SimpleConfig);

    const KConfigGroup profilesGroup(&config, QStringLiteral("Profiles"));
    if (profilesGroup.readEntry("Count", 0) <= 0) {
        p_error = Error(i18n("No profiles found."), i18n("The file %1 does not contain any profiles.", path), Error::ERROR, this);
        return false;
    }

    beginResetModel();
    p_load(config);
    endResetModel();
    commit();
    return true;
}

void ProfileModel::p_save(KConfig &config)
{
    KConfigGroup profilesGroup(&config, QStringLiteral("Profiles"));
    profilesGroup.deleteGroup();
    profilesGroup.writeEntry("Standard", p_current_profile_index);
    profilesGroup.writeEntry("Count", p_cache.count());
    profilesGroup.writeEntry(PROFILE_MODEL_VERSION_KEY, PROFILE_MODEL_VERSION);

    for (int i = 0; i < p_cache.count(); ++i) {
        KConfigGroup subGroup(&profilesGroup, QString("Profile %1").arg(i));

        subGroup.writeEntry(PROFILE_MODEL_PROFILEINDEX_KEY, p_cache[i][PROFILE_MODEL_PROFILEINDEX_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_NAME_KEY, p_cache[i][PROFILE_MODEL_NAME_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_ICON_KEY, p_cache[i][PROFILE_MODEL_ICON_KEY]);

        subGroup.writeEntry(PROFILE_MODEL_ENCODER_SELECTED_KEY, p_cache[i][PROFILE_MODEL_ENCODER_SELECTED_KEY]);

        subGroup.writeEntry(PROFILE_MODEL_SCHEME_KEY, p_cache[i][PROFILE_MODEL_SCHEME_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_FAT32COMPATIBLE_KEY, p_cache[i][PROFILE_MODEL_FAT32COMPATIBLE_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_UNDERSCORE_KEY, p_cache[i][PROFILE_MODEL_UNDERSCORE_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_2DIGITSTRACKNUM_KEY, p_cache[i][PROFILE_MODEL_2DIGITSTRACKNUM_KEY]);

        subGroup.writeEntry(PROFILE_MODEL_SC_KEY, p_cache[i][PROFILE_MODEL_SC_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_SC_SCALE_KEY, p_cache[i][PROFILE_MODEL_SC_SCALE_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_SC_SIZE_KEY, p_cache[i][PROFILE_MODEL_SC_SIZE_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_SC_FORMAT_KEY, p_cache[i][PROFILE_MODEL_SC_FORMAT_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_SC_NAME_KEY, p_cache[i][PROFILE_MODEL_SC_NAME_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_PL_KEY, p_cache[i][PROFILE_MODEL_PL_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_PL_NAME_KEY, p_cache[i][PROFILE_MODEL_PL_NAME_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_PL_ABS_FILE_PATH_KEY, p_cache[i][PROFILE_MODEL_PL_ABS_FILE_PATH_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_PL_UTF8_KEY, p_cache[i][PROFILE_MODEL_PL_UTF8_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_LOG_KEY, p_cache[i][PROFILE_MODEL_LOG_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_LOG_NAME_KEY, p_cache[i][PROFILE_MODEL_LOG_NAME_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_HOOK_KEY, p_cache[i][PROFILE_MODEL_HOOK_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_HOOK_COMMAND_KEY, p_cache[i][PROFILE_MODEL_HOOK_COMMAND_KEY]);

        subGroup.writeEntry(PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY, p_cache[i][PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_KEY, p_cache[i][PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY, p_cache[i][PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY, p_cache[i][PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY, p_cache[i][PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY]);

        subGroup.writeEntry(PROFILE_MODEL_OUTPUT_KEY, p_cache[i][PROFILE_MODEL_OUTPUT_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_IMAGE_SCHEME_KEY, p_cache[i][PROFILE_MODEL_IMAGE_SCHEME_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_CUE_KEY, p_cache[i][PROFILE_MODEL_CUE_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_CUE_NAME_KEY, p_cache[i][PROFILE_MODEL_CUE_NAME_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_CUE_MCN_ISRC_KEY, p_cache[i][PROFILE_MODEL_CUE_MCN_ISRC_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_CTDB_REPAIR_KEY, p_cache[i][PROFILE_MODEL_CTDB_REPAIR_KEY]);
        subGroup.writeEntry(PROFILE_MODEL_CTDB_REPAIR_KEEP_ORIGINAL_KEY, p_cache[i][PROFILE_MODEL_CTDB_REPAIR_KEEP_ORIGINAL_KEY]);
    }

    // Always flush. The shared config lives until the application quits,
    // so there is no destructor that would write pending changes later
    // (e.g. after the last profile has been removed).
    config.sync();
}

int ProfileModel::p_load(KConfig &config)
{
    KConfigGroup profilesGroup(&config, QStringLiteral("Profiles"));
    clear();
    p_current_profile_index = profilesGroup.readEntry("Standard", -1);
    const int profileCount = profilesGroup.readEntry("Count", QVariant(0)).toInt();
    const int version = profilesGroup.readEntry(PROFILE_MODEL_VERSION_KEY, 1);

    for (int i = 0; i < profileCount; ++i) {
        Profile p;
        KConfigGroup subGroup(&profilesGroup, QString("Profile %1").arg(i));

        p[PROFILE_MODEL_PROFILEINDEX_KEY] = subGroup.readEntry(PROFILE_MODEL_PROFILEINDEX_KEY, DEFAULT_PROFILEINDEX);
        p[PROFILE_MODEL_NAME_KEY] = subGroup.readEntry(PROFILE_MODEL_NAME_KEY, DEFAULT_NAME);
        p[PROFILE_MODEL_ICON_KEY] = subGroup.readEntry(PROFILE_MODEL_ICON_KEY, DEFAULT_ICON);

        p[PROFILE_MODEL_ENCODER_SELECTED_KEY] = subGroup.readEntry(PROFILE_MODEL_ENCODER_SELECTED_KEY, DEFAULT_ENCODER_SELECTED);

        p[PROFILE_MODEL_SCHEME_KEY] = subGroup.readEntry(PROFILE_MODEL_SCHEME_KEY, DEFAULT_SCHEME);
        p[PROFILE_MODEL_FAT32COMPATIBLE_KEY] = subGroup.readEntry(PROFILE_MODEL_FAT32COMPATIBLE_KEY, DEFAULT_FAT32);
        p[PROFILE_MODEL_UNDERSCORE_KEY] = subGroup.readEntry(PROFILE_MODEL_UNDERSCORE_KEY, DEFAULT_UNDERSCORE);
        p[PROFILE_MODEL_2DIGITSTRACKNUM_KEY] = subGroup.readEntry(PROFILE_MODEL_2DIGITSTRACKNUM_KEY, DEFAULT_2DIGITSTRACKNUM);

        p[PROFILE_MODEL_SC_KEY] = subGroup.readEntry(PROFILE_MODEL_SC_KEY, DEFAULT_SC);
        p[PROFILE_MODEL_SC_SCALE_KEY] = subGroup.readEntry(PROFILE_MODEL_SC_SCALE_KEY, DEFAULT_SC_SCALE);
        p[PROFILE_MODEL_SC_SIZE_KEY] = subGroup.readEntry(PROFILE_MODEL_SC_SIZE_KEY, DEFAULT_SC_SIZE);
        p[PROFILE_MODEL_SC_FORMAT_KEY] = subGroup.readEntry(PROFILE_MODEL_SC_FORMAT_KEY, DEFAULT_SC_FORMAT);
        p[PROFILE_MODEL_SC_NAME_KEY] = subGroup.readEntry(PROFILE_MODEL_SC_NAME_KEY, DEFAULT_SC_NAME);
        p[PROFILE_MODEL_PL_KEY] = subGroup.readEntry(PROFILE_MODEL_PL_KEY, DEFAULT_PL);
        p[PROFILE_MODEL_PL_NAME_KEY] = subGroup.readEntry(PROFILE_MODEL_PL_NAME_KEY, DEFAULT_PL_NAME);
        p[PROFILE_MODEL_PL_ABS_FILE_PATH_KEY] = subGroup.readEntry(PROFILE_MODEL_PL_ABS_FILE_PATH_KEY, DEFAULT_PL_ABS_FILE_PATH);
        p[PROFILE_MODEL_PL_UTF8_KEY] = subGroup.readEntry(PROFILE_MODEL_PL_UTF8_KEY, DEFAULT_PL_UTF8);
        p[PROFILE_MODEL_LOG_KEY] = subGroup.readEntry(PROFILE_MODEL_LOG_KEY, DEFAULT_LOG);
        p[PROFILE_MODEL_LOG_NAME_KEY] = subGroup.readEntry(PROFILE_MODEL_LOG_NAME_KEY, DEFAULT_LOG_NAME);
        p[PROFILE_MODEL_HOOK_KEY] = subGroup.readEntry(PROFILE_MODEL_HOOK_KEY, DEFAULT_HOOK);
        p[PROFILE_MODEL_HOOK_COMMAND_KEY] = subGroup.readEntry(PROFILE_MODEL_HOOK_COMMAND_KEY, DEFAULT_HOOK_COMMAND);

        p[PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY] = subGroup.readEntry(PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY, DEFAULT_ENCODER_PARAMETERS);
        p[PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_KEY] =
            subGroup.readEntry(PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_KEY, DEFAULT_ENCODER_PARAMETERS);
        p[PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY] = subGroup.readEntry(PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY, DEFAULT_ENCODER_PARAMETERS);
        p[PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY] = subGroup.readEntry(PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY, DEFAULT_ENCODER_PARAMETERS);
        p[PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY] =
            subGroup.readEntry(PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY, DEFAULT_ENCODER_PARAMETERS);

        p[PROFILE_MODEL_OUTPUT_KEY] = subGroup.readEntry(PROFILE_MODEL_OUTPUT_KEY, DEFAULT_OUTPUT);
        p[PROFILE_MODEL_IMAGE_SCHEME_KEY] = subGroup.readEntry(PROFILE_MODEL_IMAGE_SCHEME_KEY, DEFAULT_IMAGE_SCHEME);
        p[PROFILE_MODEL_CUE_KEY] = subGroup.readEntry(PROFILE_MODEL_CUE_KEY, DEFAULT_CUE);
        p[PROFILE_MODEL_CUE_NAME_KEY] = subGroup.readEntry(PROFILE_MODEL_CUE_NAME_KEY, DEFAULT_CUE_NAME);
        p[PROFILE_MODEL_CUE_MCN_ISRC_KEY] = subGroup.readEntry(PROFILE_MODEL_CUE_MCN_ISRC_KEY, DEFAULT_CUE_MCN_ISRC);
        p[PROFILE_MODEL_CTDB_REPAIR_KEY] = subGroup.readEntry(PROFILE_MODEL_CTDB_REPAIR_KEY, DEFAULT_CTDB_REPAIR);
        p[PROFILE_MODEL_CTDB_REPAIR_KEEP_ORIGINAL_KEY] = subGroup.readEntry(PROFILE_MODEL_CTDB_REPAIR_KEEP_ORIGINAL_KEY, DEFAULT_CTDB_REPAIR_KEEP_ORIGINAL);

        if (version < PROFILE_MODEL_VERSION_ENCODER_PLUGINS)
            p_migrateLegacyProfile(p, subGroup);

        p_cache.append(p);
    }

    if (!p_cache.isEmpty()) {
        // Fall back to the first profile if the stored standard profile is
        // unset or refers to a profile index that does not exist (anymore).
        if (!indexExists(p_current_profile_index))
            p_current_profile_index = p_cache.at(0)[PROFILE_MODEL_PROFILEINDEX_KEY].toInt();
        Q_EMIT profilesRemovedOrInserted();
    } else {
        // no profiles, no current profile
        p_current_profile_index = -1;
    }

    return version;
}

void ProfileModel::p_migrateLegacyProfile(Profile &profile, const KConfigGroup &subGroup)
{
    const int legacyEncoder = profile[PROFILE_MODEL_ENCODER_SELECTED_KEY].toInt();

    // plain id remap for the encoders that still exist
    switch (legacyEncoder) {
    case 0: // LAME
        profile[PROFILE_MODEL_ENCODER_SELECTED_KEY] = (int)EncoderAssistant::LAME;
        return;
    case 2: // OPUSENC
        profile[PROFILE_MODEL_ENCODER_SELECTED_KEY] = (int)EncoderAssistant::OPUSENC;
        return;
    case 3: // FLAC
        profile[PROFILE_MODEL_ENCODER_SELECTED_KEY] = (int)EncoderAssistant::FLAC;
        return;
    case 5: // WAVE
        profile[PROFILE_MODEL_ENCODER_SELECTED_KEY] = (int)EncoderAssistant::WAVE;
        return;
    case 6: // CUSTOM
        profile[PROFILE_MODEL_ENCODER_SELECTED_KEY] = (int)EncoderAssistant::CUSTOM;
        return;
    default:
        break;
    }

    // removed external encoders (OGGENC, FAAC): rebuild as CUSTOM profile.
    // Tags and cover are written by the engine after encoding, so the
    // command only carries the quality settings.
    Parameters parameters;
    QString command, suffix;

    if (legacyEncoder == LEGACY_ENCODER_OGGENC) {
        Parameters legacy;
        legacy.fromString(subGroup.readEntry(LEGACY_OGGENC_PARAMETERS_KEY, DEFAULT_ENCODER_PARAMETERS));
        command = QStringLiteral("oggenc -q %1").arg(legacy.value(LEGACY_OGGENC_QUALITY_KEY, 6).toReal(), 0, 'f', 2);
        if (legacy.value(LEGACY_OGGENC_MINBITRATE_KEY, false).toBool())
            command += QStringLiteral(" -m %1").arg(legacy.value(LEGACY_OGGENC_MINBITRATE_VALUE_KEY, 80).toInt());
        if (legacy.value(LEGACY_OGGENC_MAXBITRATE_KEY, false).toBool())
            command += QStringLiteral(" -M %1").arg(legacy.value(LEGACY_OGGENC_MAXBITRATE_VALUE_KEY, 320).toInt());
        suffix = legacy.value(LEGACY_OGGENC_SUFFIX_KEY, "ogg").toString();
    } else if (legacyEncoder == LEGACY_ENCODER_FAAC) {
        Parameters legacy;
        legacy.fromString(subGroup.readEntry(LEGACY_FAAC_PARAMETERS_KEY, DEFAULT_ENCODER_PARAMETERS));
        command = QStringLiteral("faac -q %1").arg(legacy.value(LEGACY_FAAC_QUALITY_KEY, 160).toInt());
        suffix = legacy.value(LEGACY_FAAC_SUFFIX_KEY, "mp4").toString();
    }
    // unknown ids land here as CUSTOM with an empty command; validation
    // reports "No encoder command configured" instead of failing silently

    if (!command.isEmpty())
        command += QStringLiteral(" -o \"$o\" \"$i\"");

    parameters.setValue(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, command);
    parameters.setValue(ENCODER_CUSTOM_SUFFIX_KEY, suffix);
    profile[PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY] = parameters.toString();
    profile[PROFILE_MODEL_ENCODER_SELECTED_KEY] = (int)EncoderAssistant::CUSTOM;
}

void ProfileModel::p_migrateImageSettings()
{
    // Up to profile version 2 the image options were global settings and an
    // output mode selector switched between tracks and image. They become an
    // image profile now; the global keys are removed afterwards.
    KConfigGroup general(KSharedConfig::openConfig(), QStringLiteral("general"));
    const bool imageMode = general.readEntry("imageMode", false);

    bool hasImage = false;
    for (int row = 0; row < p_cache.count() && !hasImage; ++row)
        hasImage = isImage(row);

    if (!hasImage) {
        auto encoderOf = [this](int row) {
            return static_cast<EncoderAssistant::Encoder>(p_cache.at(row)[PROFILE_MODEL_ENCODER_SELECTED_KEY].toInt());
        };

        // based on the profile used for images so far: the current one if it
        // is lossless, else the first FLAC or WAVE profile
        int base = currentProfileRow();
        if (base != -1 && !EncoderAssistant::lossless(encoderOf(base)))
            base = -1;
        for (const EncoderAssistant::Encoder encoder : {EncoderAssistant::FLAC, EncoderAssistant::WAVE})
            for (int row = 0; row < p_cache.count() && base == -1; ++row)
                if (encoderOf(row) == encoder)
                    base = row;

        const EncoderAssistant::Encoder encoder = (base != -1)    ? encoderOf(base)
            : EncoderAssistant::available(EncoderAssistant::FLAC) ? EncoderAssistant::FLAC
                                                                  : EncoderAssistant::WAVE;
        const Profile defaults = p_new_encoder_profile(encoder, true);
        Profile p = (base != -1) ? p_cache.at(base) : defaults;
        p[PROFILE_MODEL_PROFILEINDEX_KEY] = defaults[PROFILE_MODEL_PROFILEINDEX_KEY];
        QString name = defaults[PROFILE_MODEL_NAME_KEY].toString();
        p_new_name(name);
        p[PROFILE_MODEL_NAME_KEY] = name;
        p[PROFILE_MODEL_ICON_KEY] = defaults[PROFILE_MODEL_ICON_KEY];
        p[PROFILE_MODEL_OUTPUT_KEY] = PROFILE_OUTPUT_IMAGE;
        p[PROFILE_MODEL_IMAGE_SCHEME_KEY] = general.readEntry("imageNamePattern", QStringLiteral(DEFAULT_IMAGE_SCHEME));
        p[PROFILE_MODEL_CUE_KEY] = general.readEntry("cueSheet", DEFAULT_CUE);
        p[PROFILE_MODEL_CUE_NAME_KEY] = general.readEntry("cueNamePattern", QStringLiteral(DEFAULT_CUE_NAME));
        p[PROFILE_MODEL_CUE_MCN_ISRC_KEY] = general.readEntry("cueAddMcnIsrc", DEFAULT_CUE_MCN_ISRC);
        p[PROFILE_MODEL_CTDB_REPAIR_KEY] = general.readEntry("ctdbRepair", DEFAULT_CTDB_REPAIR);
        p[PROFILE_MODEL_CTDB_REPAIR_KEEP_ORIGINAL_KEY] = general.readEntry("ctdbRepairKeepOriginal", DEFAULT_CTDB_REPAIR_KEEP_ORIGINAL);
        p_cache.append(p);
        std::sort(p_cache.begin(), p_cache.end(), lessThan);

        if (imageMode)
            p_current_profile_index = p[PROFILE_MODEL_PROFILEINDEX_KEY].toInt();
    }

    for (const char *key : {"imageMode", "imageNamePattern", "cueSheet", "cueNamePattern", "cueAddMcnIsrc", "ctdbRepair", "ctdbRepairKeepOriginal"})
        general.deleteEntry(key);
    general.sync();
}
