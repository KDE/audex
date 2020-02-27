/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
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

#ifndef CDDAPARANOIA_HEADER
#define CDDAPARANOIA_HEADER

#include <QMutex>
#include <QObject>
#include <QString>

#include <KLocalizedString>

extern "C" {
#include <cdda_interface.h>
#include <cdda_paranoia.h>
}

// from cdda_interface.h
#define CD_FRAMESIZE_RAW 2352
#define CDROM_FRAMESIZE_RAW 2048
#define PREGAP 150

// from cdda_paranoia.h
#define PARANOIA_CB_READ 0
#define PARANOIA_CB_VERIFY 1
#define PARANOIA_CB_FIXUP_EDGE 2
#define PARANOIA_CB_FIXUP_ATOM 3
#define PARANOIA_CB_SCRATCH 4
#define PARANOIA_CB_REPAIR 5
#define PARANOIA_CB_SKIP 6
#define PARANOIA_CB_DRIFT 7
#define PARANOIA_CB_BACKOFF 8
#define PARANOIA_CB_OVERLAP 9
#define PARANOIA_CB_FIXUP_DROPPED 10
#define PARANOIA_CB_FIXUP_DUPED 11
#define PARANOIA_CB_READERR 12

class CDDAParanoia : public QObject
{
    Q_OBJECT

public:
    explicit CDDAParanoia(QObject *parent = nullptr);
    ~CDDAParanoia() override;

    bool setDevice(const QString &device = "/dev/cdrom");
    QString device() const;

    void setParanoiaMode(int mode); /* default: 3 */
    void setNeverSkip(bool b);
    void setMaxRetries(int m); /* default: 20 */

    qint16 *paranoiaRead(void (*callback)(long, int));
    int paranoiaSeek(long sector, int mode);

    int firstSectorOfTrack(int track);
    int lastSectorOfTrack(int track);

    int firstSectorOfDisc();
    int lastSectorOfDisc();

    void sampleOffset(const int offset);

    int numOfTracks();
    int numOfAudioTracks();

    int length();
    int numOfFrames();         // whole disc
    int lengthOfAudioTracks(); // length of all audio tracks
    int numOfFramesOfAudioTracks();
    /*sum skipped (because it is an audio track) frames
      usually used to calculate overall percent*/
    int numOfSkippedFrames(int n = 100);

    int lengthOfTrack(int n);
    int numOfFramesOfTrack(int n);
    double sizeOfTrack(int n); // in MiB
    int frameOffsetOfTrack(int n);
    bool isAudioTrack(int n);

    // First element is first track after lead-in, list of offsets, last element offset of lead-out
    // PREGAP is 150 frames = 2 seconds
    QList<quint32> discSignature(const qint32 pregap = PREGAP);

    void reset();

signals:
    void error(const QString &message, const QString &details = QString());

private:
    QMutex mutex;

    QString _device;

    cdrom_drive *paranoia_drive;
    cdrom_paranoia *paranoia;
    int paranoia_mode;
    bool paranoia_never_skip;
    int paranoia_max_retries;

    bool _paranoia_init();
    void _paranoia_free();
};

#endif
