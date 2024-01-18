/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CDDAMODEL_H
#define CDDAMODEL_H

#include <QAbstractTableModel>
#include <QBuffer>
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

#include <KCDDB/CDInfo>
#include <KCDDB/Client>
#include <KCDDB/KCDDB>

#include "utils/cddacdio.h"
#include "utils/cddadevices.h"

#include "utils/error.h"

#define CDDA_MODEL_COLUMN_RIP_LABEL i18n("Rip")
#define CDDA_MODEL_COLUMN_TRACK_LABEL i18n("Track")
#define CDDA_MODEL_COLUMN_ARTIST_LABEL i18n("Artist")
#define CDDA_MODEL_COLUMN_TITLE_LABEL i18n("Title")
#define CDDA_MODEL_COLUMN_LENGTH_LABEL i18n("Length")

enum CDDAColumms {
    CDDA_MODEL_COLUMN_RIP_INDEX = 0,
    CDDA_MODEL_COLUMN_TRACK_INDEX,
    CDDA_MODEL_COLUMN_ARTIST_INDEX,
    CDDA_MODEL_COLUMN_TITLE_INDEX,
    CDDA_MODEL_COLUMN_LENGTH_INDEX,

    CDDA_MODEL_COLUMN_COUNT
};

#define CDDA_MODEL_INTERNAL_ROLE 1982

/** kde audio disc model **/
class CDDAModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CDDAModel(QObject *parent = nullptr);
    ~CDDAModel() override;

    inline CDDACDIO *cdio() const
    {
        return p_cdio;
    }
    inline const QString deviceFile() const
    {
        return device_file;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setArtist(const QString &a);
    const QString artist() const;
    void setTitle(const QString &t);
    const QString title() const;

    // category must be cddb compatible
    //(blues, classical, country, data,
    // folk, jazz, misc, newage, reggae,
    // rock, soundtrack)
    void setCategory(const QString &c);
    const QString category() const;

    void setGenre(const QString &g);
    const QString genre() const;
    void setYear(const QString &year);
    const QString year() const;
    void setExtendedData(const QStringList &e);
    const QStringList extendedData() const;
    void setCDNum(const int n);
    int cdNum() const;
    void setTrackOffset(const int n);
    int trackOffset() const;

    int guessMultiCD(QString &newTitle) const;
    void setMultiCD(const bool multi);
    bool isMultiCD() const;

    void setCustomData(const QString &type, const QVariant &data);
    const QVariant customData(const QString &type) const;

    void setCustomDataPerTrack(const int n, const QString &type, const QVariant &data);
    const QVariant getCustomDataPerTrack(const int n, const QString &type);

    const QImage &cover() const;
    bool setCover(const QByteArray &data);
    bool setCover(const QString &filename);
    bool saveCoverToFile(const QString &filename);
    bool isCoverEmpty() const;
    void clearCover();
    const QString coverSupportedMimeTypeList() const;

    bool guessVarious() const;
    void setVarious(bool various);
    bool isVarious();

    void swapArtistAndTitleOfTracks();
    void swapArtistAndTitle();
    void splitTitleOfTracks(const QString &divider);
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

    const QList<quint32> discSignature() const;

    bool isAudioTrack(int n) const;

    void clear();

    inline bool empty()
    {
        return p_empty;
    }

    inline const QSet<int> &selectedTracks() const
    {
        return sel_tracks;
    }
    void toggle(int row);
    bool isTrackInSelection(int n) const;
    void invertSelection();
    void selectAll();
    void selectNone();

    bool isModified() const;
    void confirm();

    Error lastError() const;

public Q_SLOTS:
    void lookupCDDB();
    bool submitCDDB();

    void eject();

Q_SIGNALS:
    void audioDiscDetected();
    void audioDiscRemoved();

    void cddbLookupStarted();
    void cddbLookupDone(const bool successful);

    void cddbDataModified();
    void cddbDataSubmited(const bool successful);

    void hasSelection(bool has_selection);
    void selectionChanged(const int num_selected);

private Q_SLOTS:
    void new_audio_disc_available(const QString &udi);
    void audio_disc_removed(const QString &udi);

    void disc_information_modified();

    void lookup_cddb_done(KCDDB::Result result);

private:
    QString device_file;
    QString udi;
    CDDACDIO *p_cdio;
    CDDADevices *devices;

    KCDDB::Client *cddb;
    KCDDB::CDInfo cd_info;
    QImage p_cover;
    bool modified;
    bool p_empty; // no metadata available yet
    Error error;
    bool cddb_transaction_pending;

    QSet<int> sel_tracks;
    void p_toggle(const unsigned int track);

    const QString capitalize(const QString &s);

    void set_default_values();

    void modify();
};

#endif
