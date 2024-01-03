/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEMODEL_HEADER
#define PROFILEMODEL_HEADER

#include <climits>

#include <QAbstractTableModel>
#include <QString>
#include <QVariant>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include "utils/encoderassistant.h"
#include "utils/error.h"

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

#define DEFAULT_PL true
#define DEFAULT_PL_FORMAT "M3U"
#define DEFAULT_PL_NAME "$" VAR_ALBUM_TITLE ".$" VAR_SUFFIX
#define DEFAULT_PL_ABS_FILE_PATH false
#define DEFAULT_PL_UTF8 true

#define DEFAULT_INF false
#define DEFAULT_INF_TEXT QStringList()
#define DEFAULT_INF_NAME "info"
#define DEFAULT_INF_SUFFIX "nfo"

#define DEFAULT_HL false
#define DEFAULT_HL_FORMAT "SFV"
#define DEFAULT_HL_NAME "checksums.$" VAR_SUFFIX

#define DEFAULT_CUE false
#define DEFAULT_CUE_NAME "$" VAR_ALBUM_ARTIST " - $" VAR_ALBUM_TITLE ".$" VAR_SUFFIX

#define DEFAULT_SF false
#define DEFAULT_SF_NAME "$" VAR_ALBUM_ARTIST "/$" VAR_ALBUM_TITLE "/$" VAR_ALBUM_ARTIST " - $" VAR_ALBUM_TITLE ".$" VAR_SUFFIX

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
    PROFILE_MODEL_COLUMN_PL_FORMAT_INDEX,
    PROFILE_MODEL_COLUMN_PL_NAME_INDEX,
    PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX,
    PROFILE_MODEL_COLUMN_PL_UTF8_INDEX,

    PROFILE_MODEL_COLUMN_INF_INDEX,
    PROFILE_MODEL_COLUMN_INF_TEXT_INDEX,
    PROFILE_MODEL_COLUMN_INF_NAME_INDEX,
    PROFILE_MODEL_COLUMN_INF_SUFFIX_INDEX,

    PROFILE_MODEL_COLUMN_HL_INDEX,
    PROFILE_MODEL_COLUMN_HL_FORMAT_INDEX,
    PROFILE_MODEL_COLUMN_HL_NAME_INDEX,

    PROFILE_MODEL_COLUMN_CUE_INDEX,
    PROFILE_MODEL_COLUMN_CUE_NAME_INDEX,

    PROFILE_MODEL_COLUMN_SF_INDEX,
    PROFILE_MODEL_COLUMN_SF_NAME_INDEX,

    PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_INDEX,
    PROFILE_MODEL_COLUMN_ENCODER_OGGENC_PARAMETERS_INDEX,
    PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_INDEX,
    PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX,
    PROFILE_MODEL_COLUMN_ENCODER_FAAC_PARAMETERS_INDEX,
    PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX,
    PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX,

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
#define PROFILE_MODEL_PL_FORMAT_KEY "pl_format"
#define PROFILE_MODEL_PL_NAME_KEY "pl_name"
#define PROFILE_MODEL_PL_ABS_FILE_PATH_KEY "pl_abs_file_path"
#define PROFILE_MODEL_PL_UTF8_KEY "pl_utf8"

#define PROFILE_MODEL_INF_KEY "inf"
#define PROFILE_MODEL_INF_TEXT_KEY "inf_text"
#define PROFILE_MODEL_INF_NAME_KEY "inf_name"
#define PROFILE_MODEL_INF_SUFFIX_KEY "inf_suffix"

#define PROFILE_MODEL_HL_KEY "hl"
#define PROFILE_MODEL_HL_FORMAT_KEY "hl_format"
#define PROFILE_MODEL_HL_NAME_KEY "hl_name"

#define PROFILE_MODEL_CUE_KEY "cue"
#define PROFILE_MODEL_CUE_NAME_KEY "cue_name"

#define PROFILE_MODEL_SF_KEY "sf"
#define PROFILE_MODEL_SF_NAME_KEY "sf_name"

#define PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY "lame_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_OGGENC_PARAMETERS_KEY "oggenc_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_KEY "opusenc_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY "flac_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_FAAC_PARAMETERS_KEY "faac_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY "wave_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY "custom_parameters"

typedef QMap<QString, QVariant> Profile;

/** audex profile model **/
class ProfileModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit ProfileModel(QObject* parent = nullptr);
    ~ProfileModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    int currentProfileIndex() const;
    int currentProfileRow() const;
    int getRowByIndex(int profile_index) const;

    void clear();

    bool nameExists(const QString& name) const;
    bool indexExists(int profile_index) const;
    int getNewIndex() const;

    void sortItems();

    /**BEGIN: EncoderAssistant related */
    void autoCreate(); // scans the system for encoders and create standard profiles
    EncoderAssistant::Encoder getSelectedEncoderFromCurrentIndex();
    const Parameters getSelectedEncoderParametersFromCurrentIndex();
    const QString getSelectedEncoderSchemeFromCurrentIndex();
    const QString getSelectedEncoderSuffixFromCurrentIndex();
    const QString getSelectedEncoderNameAndVersion();
    /**END: EncoderAssistant related */

    Error lastError() const;

public Q_SLOTS:
    void commit();
    void revert() override;

    int copy(const int profileRow);

    bool saveProfilesToFile(const QString& filename);
    bool loadProfilesFromFile(const QString& filename);

    void setCurrentProfileIndex(int profile_index);
    int setRowAsCurrentProfileIndex(int row); // returns profile index

Q_SIGNALS:
    void profilesRemovedOrInserted();
    void currentProfileIndexChanged(int index);

private:
    const Profile p_new_profile();
    QList<Profile> p_cache;
    int p_current_profile_index;

    Error p_error;

    void p_new_name(QString& name);

    void p_save(KConfig* config);
    void p_load(KConfig* config);
};

#endif
