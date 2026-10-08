/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "utils/encoderassistant.h"
#include "utils/error.h"
#include "utils/parameters.h"
#include "utils/schemeparser.h"

#include <KConfig>
#include <KConfigGroup>

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QVariant>

#define DEFAULT_PROFILEINDEX -1
#define DEFAULT_NAME ""
#define DEFAULT_ICON "audio-x-generic"

#define DEFAULT_ENCODER_SELECTED 0

#define DEFAULT_ENCODER_PARAMETERS ""

#define DEFAULT_SCHEME "$" VAR_ALBUM_ARTIST "/$" VAR_ALBUM_TITLE "/$" VAR_TRACK_NO " - $" VAR_TRACK_TITLE ".$" VAR_SUFFIX

#define DEFAULT_FAT32 false
#define DEFAULT_UNDERSCORE false
#define DEFAULT_2DIGITSTRACKNUM true

#define DEFAULT_SC true
#define DEFAULT_SC_SCALE false
#define DEFAULT_SC_SIZE QSize(600, 600)
#define DEFAULT_SC_FORMAT "JPEG"
#define DEFAULT_SC_NAME "$" VAR_ALBUM_TITLE ".$" VAR_SUFFIX

#define DEFAULT_PL false
#define DEFAULT_PL_NAME "$" VAR_ALBUM_TITLE ".$" VAR_SUFFIX
#define DEFAULT_PL_ABS_FILE_PATH false
#define DEFAULT_PL_UTF8 true

#define DEFAULT_LOG false
#define DEFAULT_LOG_NAME "$" VAR_ALBUM_ARTIST " - $" VAR_ALBUM_TITLE ".$" VAR_SUFFIX

#define DEFAULT_HOOK false
#define DEFAULT_HOOK_COMMAND ""

// output of a profile: one file per track or one image file of the whole disc
#define PROFILE_OUTPUT_TRACKS 0
#define PROFILE_OUTPUT_IMAGE 1
#define DEFAULT_OUTPUT PROFILE_OUTPUT_TRACKS

#define DEFAULT_IMAGE_ICON "media-optical-audio"

#define DEFAULT_IMAGE_SCHEME "$" VAR_ALBUM_ARTIST "/$" VAR_ALBUM_TITLE "/$" VAR_ALBUM_ARTIST " - $" VAR_ALBUM_TITLE ".$" VAR_SUFFIX
#define DEFAULT_CUE true
#define DEFAULT_CUE_NAME "$" VAR_ALBUM_ARTIST " - $" VAR_ALBUM_TITLE ".$" VAR_SUFFIX
#define DEFAULT_CUE_MCN_ISRC false
#define DEFAULT_CTDB_REPAIR false
#define DEFAULT_CTDB_REPAIR_KEEP_ORIGINAL false

enum ProfileColumns {
    PROFILE_MODEL_COLUMN_PROFILEINDEX_INDEX = 0,
    PROFILE_MODEL_COLUMN_NAME_INDEX,
    PROFILE_MODEL_COLUMN_ICON_INDEX,

    PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX,

    PROFILE_MODEL_COLUMN_SCHEME_INDEX,

    PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX,
    PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX,
    PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX,

    PROFILE_MODEL_COLUMN_SC_INDEX,
    PROFILE_MODEL_COLUMN_SC_SCALE_INDEX,
    PROFILE_MODEL_COLUMN_SC_SIZE_INDEX,
    PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX,
    PROFILE_MODEL_COLUMN_SC_NAME_INDEX,

    PROFILE_MODEL_COLUMN_PL_INDEX,
    PROFILE_MODEL_COLUMN_PL_NAME_INDEX,
    PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX,
    PROFILE_MODEL_COLUMN_PL_UTF8_INDEX,

    PROFILE_MODEL_COLUMN_LOG_INDEX,
    PROFILE_MODEL_COLUMN_LOG_NAME_INDEX,

    PROFILE_MODEL_COLUMN_HOOK_INDEX,
    PROFILE_MODEL_COLUMN_HOOK_COMMAND_INDEX,

    PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_INDEX,
    PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_INDEX,
    PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX,
    PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX,
    PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX,

    PROFILE_MODEL_COLUMN_OUTPUT_INDEX,
    PROFILE_MODEL_COLUMN_IMAGE_SCHEME_INDEX,
    PROFILE_MODEL_COLUMN_CUE_INDEX,
    PROFILE_MODEL_COLUMN_CUE_NAME_INDEX,
    PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX,
    PROFILE_MODEL_COLUMN_CTDB_REPAIR_INDEX,
    PROFILE_MODEL_COLUMN_CTDB_REPAIR_KEEP_ORIGINAL_INDEX,

    PROFILE_MODEL_COLUMN_NUM
};

#define PROFILE_MODEL_PROFILEINDEX_KEY "profile_key"
#define PROFILE_MODEL_NAME_KEY "name"
#define PROFILE_MODEL_ICON_KEY "icon"
#define PROFILE_MODEL_ENCODER_SELECTED_KEY "current_encoder"

#define PROFILE_MODEL_SCHEME_KEY "scheme"

#define PROFILE_MODEL_FAT32COMPATIBLE_KEY "fat32_compatible"
#define PROFILE_MODEL_UNDERSCORE_KEY "underscore"
#define PROFILE_MODEL_2DIGITSTRACKNUM_KEY "2_digits_tracknum"

#define PROFILE_MODEL_SC_KEY "sc"
#define PROFILE_MODEL_SC_SCALE_KEY "sc_scale"
#define PROFILE_MODEL_SC_SIZE_KEY "sc_size"
#define PROFILE_MODEL_SC_FORMAT_KEY "sc_format"
#define PROFILE_MODEL_SC_NAME_KEY "sc_name"

#define PROFILE_MODEL_PL_KEY "pl"
#define PROFILE_MODEL_PL_NAME_KEY "pl_name"
#define PROFILE_MODEL_PL_ABS_FILE_PATH_KEY "pl_abs_file_path"
#define PROFILE_MODEL_PL_UTF8_KEY "pl_utf8"

#define PROFILE_MODEL_LOG_KEY "log"
#define PROFILE_MODEL_LOG_NAME_KEY "log_name"

#define PROFILE_MODEL_HOOK_KEY "hook"
#define PROFILE_MODEL_HOOK_COMMAND_KEY "hook_command"

#define PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY "lame_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_KEY "opusenc_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY "flac_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY "wave_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY "custom_parameters"

#define PROFILE_MODEL_OUTPUT_KEY "output"
#define PROFILE_MODEL_IMAGE_SCHEME_KEY "image_scheme"
#define PROFILE_MODEL_CUE_KEY "cue"
#define PROFILE_MODEL_CUE_NAME_KEY "cue_name"
#define PROFILE_MODEL_CUE_MCN_ISRC_KEY "cue_mcn_isrc"
#define PROFILE_MODEL_CTDB_REPAIR_KEY "ctdb_repair"
#define PROFILE_MODEL_CTDB_REPAIR_KEEP_ORIGINAL_KEY "ctdb_repair_keep_original"

// profile format version, stored in the "Profiles" group (absent = legacy 1.x;
// 2 = before the output type, image rips used global settings)
#define PROFILE_MODEL_VERSION_KEY "version"
#define PROFILE_MODEL_VERSION 3
#define PROFILE_MODEL_VERSION_ENCODER_PLUGINS 2 // first version with the current encoder ids

// legacy Audex 1.x profile format (external command encoders, pre-plugin engine)
#define LEGACY_ENCODER_OGGENC 1
#define LEGACY_ENCODER_FAAC 4

#define LEGACY_OGGENC_PARAMETERS_KEY "oggenc_parameters"
#define LEGACY_OGGENC_QUALITY_KEY "quality"
#define LEGACY_OGGENC_MINBITRATE_KEY "minbitrate"
#define LEGACY_OGGENC_MINBITRATE_VALUE_KEY "minbitrate_value"
#define LEGACY_OGGENC_MAXBITRATE_KEY "maxbitrate"
#define LEGACY_OGGENC_MAXBITRATE_VALUE_KEY "maxbitrate_value"
#define LEGACY_OGGENC_SUFFIX_KEY "suffix"

#define LEGACY_FAAC_PARAMETERS_KEY "faac_parameters"
#define LEGACY_FAAC_QUALITY_KEY "quality"
#define LEGACY_FAAC_SUFFIX_KEY "suffix"

typedef QMap<QString, QVariant> Profile;

/** audex profile model **/
class ProfileModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ProfileModel(QObject *parent = nullptr);
    ~ProfileModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    bool validateData(const QModelIndex &index, const QVariant &value);

    int currentProfileRow() const;
    int getRowByIndex(int profile_index) const;

    bool isImage(int row) const; // the profile rips the whole disc into one image file
    bool isAvailable(int row) const; // its encoder is available (plugin installed)
    QString unavailableReason(int row) const; // empty if available

    // Makes an available profile the current one if the current one is not.
    // The previous choice is remembered and restored as soon as it is
    // available again (e.g. after installing the missing plugin).
    void ensureAvailableCurrentProfile();

    void clear();

    bool nameExists(const QString &name) const;
    bool indexExists(int profile_index) const;
    int getNewIndex() const;

    void sortItems();

    /**BEGIN: EncoderAssistant related */
    void autoCreate(); // scans the system for encoders and create standard profiles
    EncoderAssistant::Encoder getSelectedEncoderFromCurrentIndex();
    const Parameters getSelectedEncoderParametersFromCurrentIndex();
    const QString getSelectedEncoderSuffixFromCurrentIndex();
    const QString getSelectedEncoderNameAndVersion();
    bool isSelectedEncoderWithEmbedCover();
    bool isCurrentImage() const;
    /**END: EncoderAssistant related */

    Error lastError() const;

public Q_SLOTS:
    void commit();
    void revert() override;

    int copy(const int profileRow);

    bool saveProfilesToFile(const QString &filename);
    bool loadProfilesFromFile(const QString &filename);

    void setCurrentProfileIndex(int profile_index);
    int setRowAsCurrentProfileIndex(int row); // returns profile index, -1 for an invalid row

Q_SIGNALS:
    void profilesRemovedOrInserted();
    void currentProfileIndexChanged(int index);

private:
    const Profile p_new_profile();
    Profile p_new_encoder_profile(EncoderAssistant::Encoder encoder, bool image);
    QList<Profile> p_cache;
    int p_current_profile_index;

    Error p_error;

    void p_new_name(QString &name);

    void p_save(KConfig &config);
    int p_load(KConfig &config); // returns the format version that was stored
    void p_migrateLegacyProfile(Profile &profile, const KConfigGroup &subGroup);
    void p_migrateImageSettings();
};

// Profile selection of the main window: profiles whose encoder is not
// available are shown, but disabled (the settings list shows them enabled,
// so that they can still be edited, copied or removed)
class ProfileFilterModel : public QSortFilterProxyModel
{
public:
    explicit ProfileFilterModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        Qt::ItemFlags f = QSortFilterProxyModel::flags(index);
        const auto *profiles = qobject_cast<const ProfileModel *>(sourceModel());
        if (profiles && index.isValid() && !profiles->isAvailable(mapToSource(index).row()))
            f &= ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        return f;
    }
};
