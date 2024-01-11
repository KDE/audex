/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CDDAPARANOIA_HEADER
#define CDDAPARANOIA_HEADER

#include <QMutex>
#include <QObject>
#include <QString>

#include <KLocalizedString>

extern "C" {
#include <cdio/cdio.h>
#include <cdio/paranoia/paranoia.h>
}

#define PREGAP 150

#define SECTORS_PER_SECOND 75
#define FRAMES_PER_SECOND 75

class CDDAParanoia : public QObject
{
    Q_OBJECT

public:
    explicit CDDAParanoia(QObject *parent = nullptr);
    ~CDDAParanoia() override;

    bool setDevice(const QString &device = "/dev/cdrom");
    QString device() const;

    void enableFullParanoiaMode(const bool enabled = true);
    void enableNeverSkip(const bool never_skip = true);
    void setMaxRetries(int m); /* default: 20 */

    qint16 *paranoiaRead(void (*callback)(long, paranoia_cb_mode_t));
    int paranoiaSeek(long sector, qint32 mode);

    int firstSectorOfTrack(track_t track);
    int lastSectorOfTrack(track_t track);

    int firstSectorOfDisc();
    int lastSectorOfDisc();

    void sampleOffset(const int offset);

    track_t numOfTracks() const;
    track_t numOfAudioTracks() const;

    int length() const;
    int numOfFrames() const; // whole disc
    int lengthOfAudioTracks() const; // length of all audio tracks in seconds
    int numOfFramesOfAudioTracks() const;

    // sum of skipped frames of non-audio tracks
    // (usually used to calculate overall percent)
    int numOfSkippedFrames(int n = 100) const;

    int lengthOfTrack(track_t track) const; // in seconds
    int numOfFramesOfTrack(track_t track) const;
    double sizeOfTrack(track_t track) const; // in MiB
    int frameOffsetOfTrack(track_t track) const;
    bool isAudioTrack(track_t track) const;

    // First element is first track after lead-in, list of offsets, last element offset of lead-out
    // PREGAP is 150 frames = 2 seconds
    QList<quint32> discSignature(const qint32 pregap = PREGAP);

    void reset();

Q_SIGNALS:
    void error(const QString &message, const QString &details = QString());

private:
    QMutex mutex;

    QString _device;

    cdrom_drive_t *drive;
    cdrom_paranoia_t *paranoia;
    int paranoia_mode;
    bool paranoia_never_skip;
    int paranoia_max_retries;

    bool _paranoia_init();
    void _paranoia_free();
};

#endif
