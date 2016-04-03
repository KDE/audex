/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <http://userbase.kde.org/Audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PROFILEMODEL_HEADER
#define PROFILEMODEL_HEADER

#include <limits.h>

#include <QAbstractTableModel>
#include <QVariant>
#include <QString>

#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>

#include "utils/error.h"
#include "utils/encoderassistant.h"

#define DEFAULT_PROFILEINDEX		-1
#define DEFAULT_NAME			""
#define DEFAULT_ICON			"audio-x-generic"

#define DEFAULT_ENCODER_SELECTED	0

#define DEFAULT_ENCODER_PARAMETERS	""

#define DEFAULT_PATTERN			"$" VAR_ALBUM_ARTIST"/$" VAR_ALBUM_TITLE"/$" VAR_TRACK_NO" - $" VAR_TRACK_TITLE".$" VAR_SUFFIX

#define DEFAULT_FAT32			false
#define DEFAULT_UNDERSCORE		false
#define DEFAULT_2DIGITSTRACKNUM		true

#define DEFAULT_SC			true
#define DEFAULT_SC_SCALE		false
#define DEFAULT_SC_SIZE			QSize(600, 600)
#define DEFAULT_SC_FORMAT		"JPEG"
#define DEFAULT_SC_NAME			"$" VAR_ALBUM_TITLE".$" VAR_SUFFIX

#define DEFAULT_PL			true
#define DEFAULT_PL_FORMAT		"M3U"
#define DEFAULT_PL_NAME			"$" VAR_ALBUM_TITLE".$" VAR_SUFFIX
#define DEFAULT_PL_ABS_FILE_PATH	false
#define DEFAULT_PL_UTF8                 true

#define DEFAULT_INF			false
#define DEFAULT_INF_TEXT		QStringList()
#define DEFAULT_INF_NAME		"info"
#define DEFAULT_INF_SUFFIX		"nfo"

#define DEFAULT_HL			false
#define DEFAULT_HL_FORMAT		"SFV"
#define DEFAULT_HL_NAME			"checksums.$" VAR_SUFFIX

#define DEFAULT_CUE			false
#define DEFAULT_CUE_NAME		"$" VAR_ALBUM_ARTIST" - $" VAR_ALBUM_TITLE".$" VAR_SUFFIX

#define DEFAULT_SF			false
#define DEFAULT_SF_NAME			"$" VAR_ALBUM_ARTIST"/$" VAR_ALBUM_TITLE"/$" VAR_ALBUM_ARTIST" - $" VAR_ALBUM_TITLE".$" VAR_SUFFIX


enum ProfileColumns {

  PROFILE_MODEL_COLUMN_PROFILEINDEX_INDEX = 0,
  PROFILE_MODEL_COLUMN_NAME_INDEX,
  PROFILE_MODEL_COLUMN_ICON_INDEX,

  PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX,

  PROFILE_MODEL_COLUMN_PATTERN_INDEX,

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
  PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX,
  PROFILE_MODEL_COLUMN_ENCODER_FAAC_PARAMETERS_INDEX,
  PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX,
  PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX,

  PROFILE_MODEL_COLUMN_NUM

};

#define PROFILE_MODEL_PROFILEINDEX_KEY			"profile_key"
#define PROFILE_MODEL_NAME_KEY				"name"
#define PROFILE_MODEL_ICON_KEY 				"icon"
#define PROFILE_MODEL_ENCODER_SELECTED_KEY		"current_encoder"

#define PROFILE_MODEL_PATTERN_KEY			"pattern"

#define PROFILE_MODEL_FAT32COMPATIBLE_KEY		"fat32_compatible"
#define PROFILE_MODEL_UNDERSCORE_KEY			"underscore"
#define PROFILE_MODEL_2DIGITSTRACKNUM_KEY		"2_digits_tracknum"

#define PROFILE_MODEL_SC_KEY				"sc"
#define PROFILE_MODEL_SC_SCALE_KEY			"sc_scale"
#define PROFILE_MODEL_SC_SIZE_KEY			"sc_size"
#define PROFILE_MODEL_SC_FORMAT_KEY			"sc_format"
#define PROFILE_MODEL_SC_NAME_KEY			"sc_name"

#define PROFILE_MODEL_PL_KEY				"pl"
#define PROFILE_MODEL_PL_FORMAT_KEY			"pl_format"
#define PROFILE_MODEL_PL_NAME_KEY			"pl_name"
#define PROFILE_MODEL_PL_ABS_FILE_PATH_KEY		"pl_abs_file_path"
#define PROFILE_MODEL_PL_UTF8_KEY                       "pl_utf8"

#define PROFILE_MODEL_INF_KEY				"inf"
#define PROFILE_MODEL_INF_TEXT_KEY			"inf_text"
#define PROFILE_MODEL_INF_NAME_KEY			"inf_name"
#define PROFILE_MODEL_INF_SUFFIX_KEY			"inf_suffix"

#define PROFILE_MODEL_HL_KEY				"hl"
#define PROFILE_MODEL_HL_FORMAT_KEY			"hl_format"
#define PROFILE_MODEL_HL_NAME_KEY			"hl_name"

#define PROFILE_MODEL_CUE_KEY				"cue"
#define PROFILE_MODEL_CUE_NAME_KEY			"cue_name"

#define PROFILE_MODEL_SF_KEY				"sf"
#define PROFILE_MODEL_SF_NAME_KEY			"sf_name"

#define PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_KEY	"lame_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_OGGENC_PARAMETERS_KEY	"oggenc_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_KEY	"flac_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_FAAC_PARAMETERS_KEY	"faac_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_KEY	"wave_parameters"
#define PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_KEY	"custom_parameters"

#define LABEL_MOBILE_QUALITY				i18n(" (Mobile Quality)")
#define LABEL_NORMAL_QUALITY				i18n(" (Normal Quality)")
#define LABEL_EXTREME_QUALITY				i18n(" (Extreme Quality)")

typedef QMap<QString,QVariant> Profile;

/** audex profile model **/
class ProfileModel : public QAbstractTableModel {
  Q_OBJECT
public:
  ProfileModel(QObject *parent = 0);
  ~ProfileModel();

  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
  bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());

  int currentProfileIndex() const;
  int currentProfileRow() const;
  int getRowByIndex(int profile_index) const;

  void clear();

  bool nameExists(const QString& name) const;
  bool indexExists(int profile_index) const;
  int getNewIndex() const;

  void sortItems();

  /**BEGIN: EncoderAssistant related */
  void autoCreate(); //scans the system for encoders and create standard profiles
  EncoderAssistant::Encoder getSelectedEncoderFromCurrentIndex();
  const Parameters getSelectedEncoderParametersFromCurrentIndex();
  const QString getSelectedEncoderPatternFromCurrentIndex();
  const QString getSelectedEncoderSuffixFromCurrentIndex();
  const QString getSelectedEncoderNameAndVersion();
  /**END: EncoderAssistant related */


  Error lastError() const;

public slots:
  void commit();
  void revert();

  int copy(const int profileRow);

  bool saveProfilesToFile(const QString& filename);
  bool loadProfilesFromFile(const QString& filename);

  void setCurrentProfileIndex(int profile_index);
  int setRowAsCurrentProfileIndex(int row); //returns profile index

signals:
  void profilesRemovedOrInserted();
  void currentProfileIndexChanged(int index);

private:
  const Profile p_new_profile();
  QList<Profile> p_cache;
  int p_current_profile_index;

  Error p_error;

  void p_new_name(QString& name);

  void p_save(KConfig *config);
  void p_load(KConfig *config);

};

#endif
