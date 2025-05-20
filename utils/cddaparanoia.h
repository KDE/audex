/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDebug>
#include <QObject>
#include <QString>

#include <KLocalizedString>

extern "C" {
#include <cdio/paranoia/paranoia.h>
}

#define SECTORS_PER_SECOND 75
#define FRAMES_PER_SECOND 75

#define CD_FRAMESIZE_SAMPLES 588

class CDDAParanoia : public QObject
{
    Q_OBJECT

public:
    explicit CDDAParanoia(const QString &deviceFile = "/dev/cdrom",
                          const bool enableParanoiaMode = true,
                          const int maxRetriesOnReadError = 20,
                          QObject *parent = nullptr);
    ~CDDAParanoia() override;

    bool init();

    void enableParanoiaMode(const bool enable = true);
    bool paranoiaModeEnabled() const;
    void setParanoiaMaxRetriesOnReadError(int max_retries); // default: 20
    int paranoiaMaxRetriesOnReadError() const;

    qint16 *paranoiaRead(void (*callback)(long, paranoia_cb_mode_t));
    int paranoiaSeek(const int sector, qint32 mode);
    bool paranoiaError(QString &errorMsg);

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

    QString device_file;

    int paranoia_mode;
    int paranoia_max_retries;
};
