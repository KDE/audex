/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CDDACDIO_HEADER
#define CDDACDIO_HEADER

#include <QObject>
#include <QString>

#include <KLocalizedString>

extern "C" {
#include <cdio/cdio.h>
#include <cdio/mmc.h>
#include <cdio/paranoia/paranoia.h>
}

#define SECTORS_PER_SECOND 75
#define FRAMES_PER_SECOND 75

#define CD_FRAMESIZE_SAMPLES 588

enum DriveCapability {
    CLOSE_TRAY = 0,
    EJECT,
    LOCK,
    SELECT_SPEED,
    SELECT_DISC,
    READ_MULTI_SESSION,
    MEDIA_CHANGED,
    READ_CDDA,
    C2_ERRS,
    READ_MODE2_FORM1,
    READ_MODE2_FORM2,
    READ_MCN,
    READ_ISRC
};

typedef QSet<DriveCapability> DriveCapabilities;

class CDDACDIO : public QObject
{
    Q_OBJECT

public:
    explicit CDDACDIO(QObject *parent = nullptr);
    ~CDDACDIO() override;

    bool init(const QString &deviceFile = "/dev/cdrom");
    void reset();

    const QString getDeviceFile() const;
    const QString getVendor() const;
    const QString getModel() const;
    const QString getRevision() const;

    const DriveCapabilities getDriveCapabilities() const;

    void enableParanoiaFullMode(const bool enabled = true);
    void enableParanoiaNeverSkip(const bool never_skip = true);
    void setParanoiaMaxRetries(int max_retries); /* default: 20 */

    qint16 *paranoiaRead(void (*callback)(long, paranoia_cb_mode_t));
    int paranoiaSeek(const int sector, qint32 mode);
    bool paranoiaError(QString &errorMsg);

    bool mediaChanged();

    int firstSectorOfTrack(const int track);
    int lastSectorOfTrack(const int track);

    int firstSectorOfDisc();
    int lastSectorOfDisc();

    int firstTrackNum();
    int lastTrackNum();

    int numOfTracks();
    int numOfAudioTracks();

    int length();
    int numOfFrames(); // whole disc
    int lengthOfAudioTracks(); // length of all audio tracks in seconds
    int numOfFramesOfAudioTracks();

    // sum of skipped frames of non-audio tracks
    // (usually used to calculate overall percent)
    int numOfSkippedFrames(int n = 100);

    int lengthOfTrack(const int track); // in seconds
    int numOfFramesOfTrack(const int track);
    qreal sizeOfTrack(const int track); // in MiB
    int frameOffsetOfTrack(const int track);

    bool isAudioTrack(const int track);
    bool isDataTrack(const int track);
    bool isLastTrack(const int track);

    bool isAudioDisc(); // all tracks are audio tracks?

    // First element is first track after lead-in, list of offsets, last element offset of lead-out
    QList<quint32> discSignature();

    const QStringList prettyTOC();

    const QString getMCN();
    const QString getISRC(const int track);

    bool isPreemphasis(const int track);

    const QString msfOfTrack(const int track);

    static const QString LSN2MSF(const int lsn, const QChar frameSeparator = QChar(':'))
    {
        qreal length = (qreal)(lsn) / (qreal)FRAMES_PER_SECOND;
        int min = (int)length / 60;
        int sec = (int)length % 60;
        int frames = lsn - (((min * 60) + sec) * FRAMES_PER_SECOND);
        return QString("%1:%2%3%4").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(frameSeparator).arg(frames, 2, 10, QChar('0'));
    }

Q_SIGNALS:
    void error(const QString &message, const QString &details = QString());

private:
    cdrom_drive_t *drive;
    cdrom_paranoia_t *paranoia;
    CdIo_t *cdio;

    QString device_file;

    QString vendor;
    QString model;
    QString revision;

    int paranoia_mode;
    bool paranoia_never_skip;
    int paranoia_max_retries;

    DriveCapabilities capabilities;

    QString mcn;
    QMap<int, QString> track_isrcs;

    void p_detect_hardware();
    void p_free();
};

#endif
