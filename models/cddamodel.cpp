/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddamodel.h"
#include <QRegularExpression>

CDDAModel::CDDAModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    pn = nullptr;
    _device.clear();
    _udi.clear();

    devices = new CDDADevices(this);
    if (!devices) {
        qDebug() << "Unable to create devices object. low mem?";
        error = Error(i18n("Unable to create devices object."), i18n("This is an internal error. Check your hardware. If all okay please make bug report."), Error::ERROR, this);
        return;
    }
    connect(devices, SIGNAL(audioDiscDetected(const QString &)), this, SLOT(new_audio_disc_available(const QString &)));
    connect(devices, SIGNAL(audioDiscRemoved(const QString &)), this, SLOT(audio_disc_removed(const QString &)));

    cddb = new KCDDB::Client();
    if (!cddb) {
        qDebug() << "Unable to create KCDDB object. Low mem?";
        error = Error(i18n("Unable to create KCDDB object."), i18n("This is an internal error. Check your hardware. If all okay please make bug report."), Error::ERROR, this);
        return;
    }
    connect(cddb, SIGNAL(finished(KCDDB::Result)), this, SLOT(lookup_cddb_done(KCDDB::Result)));

    cddb_transaction_pending = false;

    _cover = new CachedImage();

    cd_info.clear();
    modified = false;
    _empty = true;

    QTimer::singleShot(200, devices, SLOT(scanBus()));
}

CDDAModel::~CDDAModel()
{
    delete _cover;
    delete cddb;
    delete devices;

    if (pn)
        delete pn;
}

int CDDAModel::rowCount(const QModelIndex &parent) const
{
    if (!pn)
        return 0;
    return parent.isValid() ? 0 : pn->numOfTracks();
}

int CDDAModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return CDDA_MODEL_COLUMN_COUNT;
}

QVariant CDDAModel::data(const QModelIndex &index, int role) const
{
    if (!pn) {
        return QVariant();
    }

    if (!index.isValid()) {
        return QVariant();
    }

    if ((index.row() < 0) || (index.row() >= numOfTracks())) {
        return QVariant();
    }

    if (role == Qt::TextAlignmentRole) {
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }

    /*if (role == Qt::ForegroundRole) {
      switch (index.column()) {
        case CDDA_MODEL_COLUMN_ARTIST_INDEX :
              if (!isTrackArtistValid(index.row()+1)) return qVariantFromValue(QColor(Qt::gray));
              break;
        case CDDA_MODEL_COLUMN_TITLE_INDEX :
              if (!isTrackTitleValid(index.row()+1)) return qVariantFromValue(QColor(Qt::gray));
              break;
      }
    }*/

    if ((role == Qt::DisplayRole) || (role == Qt::CheckStateRole && index.column() == CDDA_MODEL_COLUMN_RIP_INDEX) || (role == CDDA_MODEL_INTERNAL_ROLE) || (role == Qt::EditRole)) {
        switch (index.column()) {
        case CDDA_MODEL_COLUMN_RIP_INDEX:
            if (role == Qt::CheckStateRole) {
                return isTrackInSelection(index.row() + 1) ? Qt::Checked : Qt::Unchecked;
            }
            break;
        case CDDA_MODEL_COLUMN_TRACK_INDEX:
            return index.row() + 1 + (trackOffset() > 1 ? trackOffset() : 0);
        case CDDA_MODEL_COLUMN_LENGTH_INDEX:
            if (role == CDDA_MODEL_INTERNAL_ROLE)
                return lengthOfTrack(index.row() + 1);
            else
                return QString("%1:%2").arg(lengthOfTrack(index.row() + 1) / 60, 2, 10, QChar('0')).arg(lengthOfTrack(index.row() + 1) % 60, 2, 10, QChar('0'));
        case CDDA_MODEL_COLUMN_ARTIST_INDEX:
            if (isAudioTrack(index.row() + 1)) {
                QString a = cd_info.track(index.row()).get(KCDDB::Artist).toString();
                return a;
            }
            break;
        case CDDA_MODEL_COLUMN_TITLE_INDEX:
            if (isAudioTrack(index.row() + 1)) {
                QString t = cd_info.track(index.row()).get(KCDDB::Title).toString();
                if (t.isEmpty())
                    return i18n("Track %1", index.row() + 1);
                return t;
            }
            break;
        default:;
        }
    }

    return QVariant();
}

bool CDDAModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!pn) {
        return false;
    }

    if (!index.isValid()) {
        return false;
    }

    if ((index.row() < 0) || (index.row() >= numOfTracks())) {
        return false;
    }

    if (role == Qt::EditRole) {
        bool changed = false;
        switch (index.column()) {
        case CDDA_MODEL_COLUMN_ARTIST_INDEX:
            if (value != cd_info.track(index.row()).get(KCDDB::Artist)) {
                cd_info.track(index.row()).set(KCDDB::Artist, value);
                changed = true;
            }
            break;
        case CDDA_MODEL_COLUMN_TITLE_INDEX:
            if (value != cd_info.track(index.row()).get(KCDDB::Title)) {
                cd_info.track(index.row()).set(KCDDB::Title, value);
                changed = true;
            }
            break;
        default:
            return false;
        }
        if (changed) {
            Q_EMIT dataChanged(index, index);
            modify();
        }
        return changed;
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
            case CDDA_MODEL_COLUMN_RIP_INDEX:
                return CDDA_MODEL_COLUMN_RIP_LABEL;
            case CDDA_MODEL_COLUMN_TRACK_INDEX:
                return CDDA_MODEL_COLUMN_TRACK_LABEL;
            case CDDA_MODEL_COLUMN_LENGTH_INDEX:
                return CDDA_MODEL_COLUMN_LENGTH_LABEL;
            case CDDA_MODEL_COLUMN_ARTIST_INDEX:
                return CDDA_MODEL_COLUMN_ARTIST_LABEL;
            case CDDA_MODEL_COLUMN_TITLE_INDEX:
                return CDDA_MODEL_COLUMN_TITLE_LABEL;
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
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if ((index.column() == CDDA_MODEL_COLUMN_ARTIST_INDEX) || (index.column() == CDDA_MODEL_COLUMN_TITLE_INDEX)) {
        flags |= Qt::ItemIsEditable;
    }
    if (!isAudioTrack(index.row() + 1))
        return Qt::ItemIsEnabled;
    return flags;
}

void CDDAModel::setArtist(const QString &a)
{
    if (!pn)
        return;
    if (a != cd_info.get(KCDDB::Artist).toString()) {
        beginResetModel();
        cd_info.set(KCDDB::Artist, a);
        modify();
        endResetModel();
    }
}

const QString CDDAModel::artist() const
{
    if (!pn)
        return QString();
    QString a = cd_info.get(KCDDB::Artist).toString();
    return a;
}

void CDDAModel::setTitle(const QString &t)
{
    if (!pn)
        return;
    if (t != cd_info.get(KCDDB::Title).toString()) {
        beginResetModel();
        cd_info.set(KCDDB::Title, t);
        modify();
        endResetModel();
    }
}

const QString CDDAModel::title() const
{
    if (!pn)
        return QString();
    QString t = cd_info.get(KCDDB::Title).toString();
    return t;
}

void CDDAModel::setCategory(const QString &c)
{
    if (!pn)
        return;

    QStringList validCategories;
    validCategories << "blues"
                    << "classical"
                    << "country"
                    << "data"
                    << "folk"
                    << "jazz"
                    << "misc"
                    << "newage"
                    << "reggae"
                    << "rock"
                    << "soundtrack";
    if (!validCategories.contains(c))
        return;

    if (c != cd_info.get(KCDDB::Category).toString()) {
        beginResetModel();
        cd_info.set(KCDDB::Category, c);
        modify();
        endResetModel();
    }
}

const QString CDDAModel::category() const
{
    if (!pn)
        return QString();
    return cd_info.get(KCDDB::Category).toString();
}

void CDDAModel::setGenre(const QString &g)
{
    if (!pn)
        return;
    if (g != cd_info.get(KCDDB::Genre).toString()) {
        beginResetModel();
        cd_info.set(KCDDB::Genre, g);
        modify();
        endResetModel();
    }
}

const QString CDDAModel::genre() const
{
    if (!pn)
        return QString();
    return cd_info.get(KCDDB::Genre).toString();
}

void CDDAModel::setYear(const QString &year)
{
    if (!pn)
        return;
    if (year != cd_info.get(KCDDB::Year).toString()) {
        beginResetModel();
        cd_info.set(KCDDB::Year, year);
        modify();
        endResetModel();
    }
}

const QString CDDAModel::year() const
{
    if (!pn)
        return QString();
    return cd_info.get(KCDDB::Year).toString();
}

void CDDAModel::setExtendedData(const QStringList &e)
{
    if (!pn)
        return;
    if (e != cd_info.get(KCDDB::Comment).toStringList()) {
        beginResetModel();
        cd_info.set(KCDDB::Comment, e);
        modify();
        endResetModel();
    }
}

const QStringList CDDAModel::extendedData() const
{
    if (!pn)
        return QStringList();
    return cd_info.get(KCDDB::Comment).toStringList();
}

void CDDAModel::setCDNum(const int n)
{
    if (!pn)
        return;
    if (n != cd_info.get("DNO").toInt()) {
        beginResetModel();
        cd_info.set("DNO", n);
        modify();
        endResetModel();
    }
}

int CDDAModel::cdNum() const
{
    if (!pn)
        return -1;
    if (!isMultiCD())
        return 0;
    return cd_info.get("DNO").toInt();
}

void CDDAModel::setTrackOffset(const int n)
{
    if (!pn)
        return;
    if (n != cd_info.get("DTRACKOFFSET").toInt()) {
        beginResetModel();
        cd_info.set("DTRACKOFFSET", n);
        modify();
        endResetModel();
    }
}

int CDDAModel::trackOffset() const
{
    if (!pn)
        return -1;
    return cd_info.get("DTRACKOFFSET").toInt();
}

int CDDAModel::guessMultiCD(QString &newTitle) const
{
    if (!pn)
        return -1;

    QString t = cd_info.get(KCDDB::Title).toString();
    static QRegularExpression rx1("[\\(|\\[]* *([c|C][d|D]|[d|D][i|I][s|S][k|c|K|C]) *[0-9]* *[\\)|\\]]* *$");
    int i = t.indexOf(rx1);
    if (i >= 0) {
        QString frac = t.mid(i);
        static QRegularExpression rx2("(\\d+)");
        auto match = rx2.match(frac);
        bool ok;
        int cdnum = match.captured(0).toInt(&ok);
        if (ok) {
            if (cdnum < 0)
                return -1;
            if (cdnum == 0)
                cdnum = 1;
            newTitle = t.left(i).trimmed();
            return cdnum;
        }
    }
    return -1;
}

void CDDAModel::setMultiCD(const bool multi)
{
    if (!pn)
        return;
    if (multi != cd_info.get("DMULTICD").toBool()) {
        beginResetModel();
        cd_info.set("DMULTICD", multi);
        modify();
        endResetModel();
    }
}

bool CDDAModel::isMultiCD() const
{
    if (!pn)
        return false;
    return cd_info.get("DMULTICD").toBool();
}

void CDDAModel::setCustomData(const QString &type, const QVariant &data)
{
    if (!pn)
        return;
    if (data != cd_info.get(type)) {
        beginResetModel();
        cd_info.set(type, data);
        modify();
        endResetModel();
    }
}

const QVariant CDDAModel::customData(const QString &type) const
{
    if (!pn)
        return QVariant();
    return cd_info.get(type);
}

void CDDAModel::setCustomDataPerTrack(const int n, const QString &type, const QVariant &data)
{
    if (!pn)
        return;
    if (data != cd_info.track(n).get(type)) {
        beginResetModel();
        cd_info.track(n).set(type, data);
        modify();
        endResetModel();
    }
}

const QVariant CDDAModel::getCustomDataPerTrack(const int n, const QString &type)
{
    if (!pn)
        return QVariant();
    return cd_info.track(n).get(type);
}

CachedImage *CDDAModel::cover() const
{
    return _cover;
}

const QImage CDDAModel::coverImage() const
{
    return _cover->coverImage();
}

const QString CDDAModel::coverChecksum() const
{
    return _cover->checksum();
}

bool CDDAModel::setCover(const QByteArray &data)
{
    if (_cover->load(data)) {
        beginResetModel();
        endResetModel();
        return true;
    } else {
        error = _cover->lastError();
    }
    return false;
}

bool CDDAModel::setCover(const QString &filename)
{
    if (_cover->load(filename)) {
        beginResetModel();
        endResetModel();
        return true;
    } else {
        error = _cover->lastError();
    }
    return false;
}

bool CDDAModel::saveCoverToFile(const QString &filename)
{
    if (_cover->save(filename)) {
        return true;
    } else {
        error = _cover->lastError();
    }
    return false;
}

bool CDDAModel::isCoverEmpty() const
{
    return _cover->isEmpty();
}

void CDDAModel::clearCover()
{
    if (_cover->isEmpty())
        return;

    beginResetModel();
    _cover->clear();
    endResetModel();
}

const QString CDDAModel::coverSupportedMimeTypeList() const
{
    return _cover->supportedMimeTypeList();
}

bool CDDAModel::guessVarious() const
{
    if (!pn)
        return false;
    QString a;
    for (int i = 0; i < cd_info.numberOfTracks(); ++i) {
        if ((i > 0) && (cd_info.track(i).get(KCDDB::Artist).toString().toLower() != a.toLower()))
            return true;
        a = cd_info.track(i).get(KCDDB::Artist).toString();
    }
    return false;
}

void CDDAModel::setVarious(bool various)
{
    if (!pn)
        return;
    if (various != cd_info.get("DVARIOUS").toBool()) {
        cd_info.set("DVARIOUS", various);
        modify();
    }
}

bool CDDAModel::isVarious()
{
    if (!pn)
        return false;
    return cd_info.get("DVARIOUS").toBool();
}

void CDDAModel::swapArtistAndTitleOfTracks()
{
    if (!pn)
        return;

    beginResetModel();
    for (int i = 0; i < cd_info.numberOfTracks(); ++i) {
        QVariant tmp = cd_info.track(i).get(KCDDB::Artist);
        cd_info.track(i).set(KCDDB::Artist, cd_info.track(i).get(KCDDB::Title));
        cd_info.track(i).set(KCDDB::Title, tmp);
    }
    modified = true;
    endResetModel();
    Q_EMIT cddbDataModified();
}

void CDDAModel::swapArtistAndTitle()
{
    if (!pn)
        return;
    QVariant tmp = cd_info.get(KCDDB::Title);
    beginResetModel();
    cd_info.set(KCDDB::Title, cd_info.get(KCDDB::Artist));
    cd_info.set(KCDDB::Artist, tmp);
    modified = true;
    endResetModel();
    Q_EMIT cddbDataModified();
}

void CDDAModel::splitTitleOfTracks(const QString &divider)
{
    if (!pn)
        return;

    beginResetModel();
    for (int i = 0; i < cd_info.numberOfTracks(); ++i) {
        int splitPos = cd_info.track(i).get(KCDDB::Title).toString().indexOf(divider);
        if (splitPos >= 0) {
            // split
            QString title = cd_info.track(i).get(KCDDB::Title).toString().mid(splitPos + divider.length());
            QString artist = cd_info.track(i).get(KCDDB::Title).toString().left(splitPos);
            cd_info.track(i).set(KCDDB::Artist, artist);
            cd_info.track(i).set(KCDDB::Title, title);
        }
    }
    modified = true;
    Q_EMIT cddbDataModified();
    endResetModel();
}

void CDDAModel::capitalizeTracks()
{
    if (!pn)
        return;

    beginResetModel();
    for (int i = 0; i < cd_info.numberOfTracks(); ++i) {
        cd_info.track(i).set(KCDDB::Artist, capitalize(cd_info.track(i).get(KCDDB::Artist).toString()));
        cd_info.track(i).set(KCDDB::Title, capitalize(cd_info.track(i).get(KCDDB::Title).toString()));
    }
    modified = true;
    endResetModel();
    Q_EMIT cddbDataModified();
}

void CDDAModel::capitalizeHeader()
{
    if (!pn)
        return;

    beginResetModel();
    cd_info.set(KCDDB::Artist, capitalize(cd_info.get(KCDDB::Artist).toString()));
    cd_info.set(KCDDB::Title, capitalize(cd_info.get(KCDDB::Title).toString()));
    modified = true;
    endResetModel();
    Q_EMIT cddbDataModified();
}

void CDDAModel::setTitleArtistsFromHeader()
{
    if (!pn)
        return;

    beginResetModel();
    for (int i = 0; i < cd_info.numberOfTracks(); ++i) {
        cd_info.track(i).set(KCDDB::Artist, cd_info.get(KCDDB::Artist));
    }
    modified = true;
    endResetModel();
    Q_EMIT cddbDataModified();
}

int CDDAModel::numOfTracks() const
{
    if (!pn)
        return 0;
    return pn->numOfTracks();
}

int CDDAModel::numOfAudioTracks() const
{
    int c = 0;
    for (int i = 1; i <= numOfTracks(); ++i) {
        if (isAudioTrack(i))
            ++c;
    }
    return c;
}

int CDDAModel::numOfAudioTracksInSelection() const
{
    return sel_tracks.count();
}

int CDDAModel::length() const
{
    if (!pn)
        return 0;
    return pn->length();
}

int CDDAModel::lengthOfAudioTracks() const
{
    int c = 0;
    for (int i = 1; i <= numOfTracks(); ++i) {
        if (isAudioTrack(i))
            c += lengthOfTrack(i);
    }
    return c;
}

int CDDAModel::lengthOfAudioTracksInSelection() const
{
    QSet<int>::ConstIterator it(sel_tracks.begin()), end(sel_tracks.end());
    int l = 0;
    for (; it != end; ++it) {
        if (isAudioTrack(*it))
            l += lengthOfTrack(*it);
    }
    return l;
}

int CDDAModel::lengthOfTrack(int n) const
{
    if (!pn)
        return 0;
    return pn->lengthOfTrack(n);
}

const QList<quint32> CDDAModel::discSignature() const
{
    if (!pn)
        return QList<quint32>();
    return pn->discSignature();
}

bool CDDAModel::isAudioTrack(int n) const
{
    if (!pn)
        return false;
    return pn->isAudioTrack(n);
}

void CDDAModel::clear()
{
    beginResetModel();
    cd_info.clear();
    clearCover();
    endResetModel();
}

void CDDAModel::toggle(int row)
{
    _toggle(row + 1);

    Q_EMIT hasSelection(0 != sel_tracks.count());
    Q_EMIT selectionChanged(sel_tracks.count());
}

bool CDDAModel::isTrackInSelection(int n) const
{
    return sel_tracks.contains(n);
}

void CDDAModel::invertSelection()
{
    for (int i = 1; i <= numOfTracks(); ++i) {
        if (isAudioTrack(i))
            _toggle(i);
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
    return modified;
}

void CDDAModel::confirm()
{
    modified = false;
}

Error CDDAModel::lastError() const
{
    return error;
}

void CDDAModel::lookupCDDB()
{
    if (!pn)
        return;

    qDebug() << "lookupCDDB called";

    if (cddb_transaction_pending) {
        qDebug() << "CDDB transaction already in progress.";
        return;
    }
    cddb_transaction_pending = true;

    Q_EMIT cddbLookupStarted();

    cddb->config().reparse();
    cddb->setBlockingMode(false);
    cddb->lookup(pn->discSignature());
}

bool CDDAModel::submitCDDB()
{
    if (!pn)
        return true;

    qDebug() << "submitCDDB called";

    if (cddb_transaction_pending) {
        qDebug() << "CDDB transaction already in progress.";
        error = Error(i18n("CDDB transaction already in progress."), i18n("A CDDB transaction is already in progress. Please wait until it has finished and try again."), Error::ERROR, this);
        return false;
    }

    cddb_transaction_pending = true;

    cddb->config().reparse();
    cddb->setBlockingMode(true);
    if (category().isEmpty()) {
        setCategory("rock");
    }
    KCDDB::Result result = cddb->submit(cd_info, pn->discSignature());

    if (result != KCDDB::Success) {
        switch (result) {
        case KCDDB::ServerError:
            error = Error(KCDDB::resultToString(result), i18n("There is an error with the CDDB server. Please wait or contact the administrator of the CDDB server."), Error::ERROR, this);
            break;
        case KCDDB::HostNotFound:
            error = Error(KCDDB::resultToString(result), i18n("Cannot find the CDDB server. Check your network. Maybe the CDDB server is offline."), Error::ERROR, this);
            break;
        case KCDDB::NoResponse:
            error = Error(KCDDB::resultToString(result), i18n("Please wait, maybe the server is busy, or contact the CDDB server administrator."), Error::ERROR, this);
            break;
        case KCDDB::CannotSave:
            error = Error(KCDDB::resultToString(result), i18n("Please contact the CDDB server administrator."), Error::ERROR, this);
            break;
        case KCDDB::InvalidCategory:
            error = Error(KCDDB::resultToString(result), i18n("This should not happen. Please make a bug report."), Error::ERROR, this);
            break;
        case KCDDB::UnknownError:;
        case KCDDB::NoRecordFound:;
        case KCDDB::MultipleRecordFound:;
        case KCDDB::Success:;
        default:
            error = Error(KCDDB::resultToString(result), i18n("Please make a bug report and contact the CDDB server administrator."), Error::ERROR, this);
            break;
        }
        return false;
    }

    error = Error();

    confirm();

    cddb_transaction_pending = false;

    Q_EMIT cddbDataSubmited(true);

    return true;
}

void CDDAModel::eject()
{
    devices->eject(_udi);
}

void CDDAModel::new_audio_disc_available(const QString &udi)
{
    if (pn)
        return;

    _device = devices->blockDevice(udi);
    _udi = udi;

    pn = new CDDAParanoia(this);
    if (!pn) {
        qDebug() << "Unable to create paranoia class. low mem?";
        error = Error(i18n("Unable to create CDDA paranoia object."), i18n("This is an internal error. Check your hardware. If all okay please make bug report."), Error::ERROR, this);
        return;
    }
    pn->setDevice(_device);

    qDebug() << "new audio disc detected (" << udi << ", " << _device << ")";

    clear();
    confirm();

    sel_tracks.clear();
    for (int i = 1; i <= pn->numOfTracks(); ++i) {
        if (isAudioTrack(i))
            sel_tracks.insert(i);
    }

    Q_EMIT hasSelection(0 != sel_tracks.size());

    Q_EMIT audioDiscDetected();
}

void CDDAModel::audio_disc_removed(const QString &udi)
{
    qDebug() << "audio disc removed (" << udi << ")";

    _device.clear();
    _udi.clear();

    if (pn)
        delete pn;
    pn = nullptr;

    Q_EMIT audioDiscRemoved();
}

void CDDAModel::disc_information_modified()
{
    qDebug() << "disc info changed";

    beginResetModel();
    set_default_values();
    setVarious(guessVarious());
    endResetModel();
}

void CDDAModel::lookup_cddb_done(KCDDB::Result result)
{
    if ((result != KCDDB::Success) && (result != KCDDB::MultipleRecordFound)) {
        switch (result) {
        case KCDDB::ServerError:
            error = Error(KCDDB::resultToString(result), i18n("There is an error with the CDDB server. Please wait or contact the administrator of the CDDB server."), Error::ERROR, this);
            break;
        case KCDDB::HostNotFound:
            error = Error(KCDDB::resultToString(result), i18n("Cannot find the CDDB server. Check your network. Maybe the CDDB server is offline."), Error::ERROR, this);
            break;
        case KCDDB::NoResponse:
            error = Error(KCDDB::resultToString(result), i18n("Please wait, maybe the server is busy, or contact the CDDB server administrator."), Error::ERROR, this);
            break;
        case KCDDB::InvalidCategory:
            error = Error(KCDDB::resultToString(result), i18n("This should not happen. Please make a bug report."), Error::ERROR, this);
            break;
        case KCDDB::UnknownError:
            error = Error(KCDDB::resultToString(result), i18n("Please make a bug report and contact the CDDB server administrator."), Error::ERROR, this);
            break;
        case KCDDB::NoRecordFound:;
        case KCDDB::MultipleRecordFound:;
        case KCDDB::Success:;
        default:
            error = Error(KCDDB::resultToString(result), i18n("This means no data found in the CDDB database."), Error::ERROR, this);
        }
        Q_EMIT cddbLookupDone(false);
        return;
    }

    KCDDB::CDInfo info = cddb->lookupResponse().constFirst();
    if (cddb->lookupResponse().count() > 1) {
        KCDDB::CDInfoList cddb_info = cddb->lookupResponse();
        KCDDB::CDInfoList::iterator it;
        QStringList list;
        for (it = cddb_info.begin(); it != cddb_info.end(); ++it) {
            list.append(QString("%1, %2, %3, %4").arg((it->get(KCDDB::Artist).toString()), it->get(KCDDB::Title).toString(), it->get(KCDDB::Genre).toString(), it->get(KCDDB::Year).toString()));
        }

        bool ok = false;
        // Uses a ComboBox, could use UseListViewForComboBoxItems if necessary
        QString res = QInputDialog::getItem(nullptr, i18n("Select CDDB Entry"), i18n("Select a CDDB entry:"), list, 0, false, &ok, {});

        if (ok) {
            // The user selected an item and pressed OK
            int c = 0;
            for (QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
                if (*it == res)
                    break;
                c++;
            }
            if (c < cddb_info.size())
                info = cddb_info[c];
        } else {
            Q_EMIT cddbLookupDone(true);
            return;
            // user pressed cancel
        }
    }

    beginResetModel();

    cd_info = info;
    set_default_values();
    setVarious(guessVarious());
    if (isVarious() && QLatin1String("Various") == artist()) {
        setArtist(i18n("Various Artists"));
    }

    QString newTitle;
    int cdnum = guessMultiCD(newTitle);
    if (cdnum > 0) {
        setMultiCD(true);
        setCDNum(cdnum);
        setTitle(newTitle);
    }
    endResetModel();

    cddb_transaction_pending = false;

    _empty = false;

    Q_EMIT cddbLookupDone(true);
}

void CDDAModel::_toggle(const unsigned int track)
{
    if (sel_tracks.contains(track)) {
        sel_tracks.remove(track);
    } else {
        sel_tracks.insert(track);
    }
}

const QString CDDAModel::capitalize(const QString &s)
{
    QStringList stringlist = s.split(' ', Qt::SkipEmptyParts);
    for (int i = 0; i < stringlist.count(); i++) {
        QString string = stringlist[i].toLower();
        int j = 0;
        while (((string[j] == '(') || (string[j] == '[') || (string[j] == '{')) && (j < string.length()))
            j++;
        string[j] = string[j].toUpper();
        stringlist[i] = string;
    }
    return stringlist.join(" ");
}

void CDDAModel::set_default_values()
{
    if (cd_info.get(KCDDB::Year).toString().isEmpty())
        cd_info.set(KCDDB::Year, QString("%1").arg(QDate::currentDate().year()));
    cd_info.set("DNO", 1);
    cd_info.set("DTRACKOFFSET", 1);
    cd_info.set("DMULTICD", false);
}

void CDDAModel::modify()
{
    modified = true;
    _empty = false;
    Q_EMIT cddbDataModified();
}
