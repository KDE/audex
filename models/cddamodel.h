/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2013 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
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

#ifndef CDDAMODEL_H
#define CDDAMODEL_H

#include <QAbstractTableModel>
#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QString>
#include <QVariant>
#include <QModelIndexList>
#include <QSet>
#include <QRegExp>

#include <KDebug>
#include <KLocale>
#include <KInputDialog>
#include <KMimeType>
#include <libkcompactdisc/kcompactdisc.h>

#include <libkcddb/kcddb.h>
#include <libkcddb/client.h>
#include <libkcddb/cdinfo.h>

#include "utils/cachedimage.h"

#include "utils/error.h"

#define CDDA_MODEL_COLUMN_RIP_LABEL	i18n("Rip")
#define CDDA_MODEL_COLUMN_TRACK_LABEL	i18n("Track")
#define CDDA_MODEL_COLUMN_ARTIST_LABEL	i18n("Artist")
#define CDDA_MODEL_COLUMN_TITLE_LABEL	i18n("Title")
#define CDDA_MODEL_COLUMN_LENGTH_LABEL	i18n("Length")

enum CDDAColumms {
  CDDA_MODEL_COLUMN_RIP_INDEX = 0,
  CDDA_MODEL_COLUMN_TRACK_INDEX,
  CDDA_MODEL_COLUMN_ARTIST_INDEX,
  CDDA_MODEL_COLUMN_TITLE_INDEX,
  CDDA_MODEL_COLUMN_LENGTH_INDEX,

  CDDA_MODEL_COLUMN_COUNT
};

#define CDDA_MODEL_INTERNAL_ROLE	1982

/** kde audio disc model **/
class CDDAModel : public QAbstractTableModel {

  Q_OBJECT

public:
  CDDAModel(QObject *parent = 0, const QString& device = "/dev/cdrom");
  ~CDDAModel();

  void setDevice(const QString& device);
  inline const QString& device() const { return _device; }

  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

  Qt::ItemFlags flags(const QModelIndex &index) const;

  unsigned int discid() const;
  void setArtist(const QString& a);
  const QString artist() const;
  void setTitle(const QString& t);
  const QString title() const;

  //category must be cddb compatible
  //(blues, classical, country, data,
  //folk, jazz, misc, newage, reggae,
  //rock, soundtrack)
  void setCategory(const QString& c);
  const QString category() const;

  void setGenre(const QString& g);
  const QString genre() const;
  void setYear(const QString& year);
  const QString year() const;
  void setExtendedData(const QStringList& e);
  const QStringList extendedData() const;
  void setCDNum(const int n);
  int cdNum() const;
  void setTrackOffset(const int n);
  int trackOffset() const;

  int guessMultiCD(QString& newTitle) const;
  void setMultiCD(const bool multi);
  bool isMultiCD() const;

  void setCustomData(const QString& type, const QVariant& data);
  const QVariant customData(const QString& type) const;

  void setCustomDataPerTrack(const int n, const QString& type, const QVariant& data);
  const QVariant getCustomDataPerTrack(const int n, const QString& type);

  CachedImage* cover() const;
  const QImage coverImage() const;
  quint16 coverChecksum() const;
  bool setCover(const QByteArray& data);
  bool setCover(const QString& filename);
  bool saveCoverToFile(const QString& filename);
  bool isCoverEmpty() const;
  void clearCover();
  const QString coverSupportedMimeTypeList() const;

  bool guessVarious() const;
  void setVarious(bool various);
  bool isVarious();

  void swapArtistAndTitleOfTracks();
  void swapArtistAndTitle();
  void splitTitleOfTracks(const QString& divider);
  void capitalizeTracks();
  void capitalizeHeader();
  void setTitleArtistsFromHeader();

  int numOfTracks() const;
  int numOfAudioTracks() const;
  int numOfAudioTracksInSelection() const;

  int length() const;
  int lengthOfAudioTracks() const;
  int lengthOfAudioTracksInSelection() const;
  int lengthOfTrack(int n) const;
  
  const QList<unsigned> discSignature() const;

  bool isAudioTrack(int n) const;

  void clear();

  inline const QSet<int>& selectedTracks() const { return sel_tracks; }
  void toggle(int row);
  bool isTrackInSelection(int n) const;
  void invertSelection();
  void selectAll();
  void selectNone();

  bool isModified() const;
  void confirm();

  Error lastError() const;

  enum DriveStatus {
    DriveNoStatus,
    DriveEmpty,
    DriveReady,
    DriveOpen,
    DriveNotReady,
    DriveError
  };

  enum DiscStatus {
    DiscNoStatus,
    DiscPlaying,
    DiscPaused,
    DiscStopped
  };

  enum DiscType {
    DiscNoType,
    DiscContainsAudioTracks,
    DiscContainsNoAudioTracks
  };

  enum DiscInfo {
    DiscNoInfo,
    DiscManualInfo, /*user input*/
    DiscCDTEXTInfo,
    DiscCDDBInfo,
    DiscPhononMetadataInfo
  };

  DriveStatus driveStatus() const;
  const QString driveStatusString() const;

  DiscStatus discStatus() const;
  const QString discStatusString() const;

  DiscType discType() const;
  const QString discTypeString() const;

  DiscInfo discInfo() const;
  const QString discInfoString() const;

public slots:
  void lookupCDDB();
  bool submitCDDB();
  void eject();

  void play(const unsigned int track = 1);
  void playPos(const unsigned int position);
  unsigned int position() const;
  void prev();
  void next();
  void pause();
  void stop();

signals:
  void driveStatusChanged(const CDDAModel::DriveStatus status);
  void discStatusChanged(const CDDAModel::DiscStatus status);
  void discChanged(const CDDAModel::DiscType type);
  void discInfoChanged(const CDDAModel::DiscInfo info);

  void cddbLookupStarted();
  void cddbLookupDone(const bool successful);
  
  void cddbDataModified();
  void cddbDataSubmited(const bool successful);

  void hasSelection(bool has_selection);
  void selectionChanged(const int num_selected);

private slots:
  void slot_disc_changed(unsigned int tracks);
  void slot_disc_information(KCompactDisc::DiscInfo info);
  void slot_disc_status_changed(KCompactDisc::DiscStatus status);

  void lookup_cddb_done(KCDDB::Result result);

private:
  QString _device;
  KCompactDisc *compact_disc;
  KCDDB::Client *cddb;
  KCDDB::CDInfo cd_info;
  CachedImage *_cover;
  bool modified;
  DriveStatus drive_status;
  DiscStatus disc_status;
  DiscType disc_type;
  DiscInfo disc_info;
  Error error;

  QSet<int> sel_tracks;
  void _toggle(const unsigned int track);

  const QString capitalize(const QString &s);

  void set_default_values();

  void modify();

};

#endif
