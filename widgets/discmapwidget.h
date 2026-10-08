/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QList>
#include <QPair>
#include <QWidget>

// Linear sector map of the current rip: tracks as segments, colored by what
// happened to them, the read position as a marker, problem sectors as ticks.
class DiscMapWidget : public QWidget
{
    Q_OBJECT

public:
    enum class TrackState {
        Pending,
        Reading, // blue fill up to the read position
        Confirmed, // green: a database confirms the track
        Unconfirmed, // yellow: not confirmed, will be read again securely
        Rereading, // yellow, blue fill up to the read position
        Done, // blue: read (securely or not), not confirmed
        Suspicious, // red: could not be read reliably
    };

    explicit DiscMapWidget(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public Q_SLOTS:
    // (sector count, track number) per track, in rip order
    void setTracks(const QList<QPair<qint64, int>> &tracks);
    // sectors of the track read by its running extraction
    void setTrackProgress(int trackNumber, qint64 sectors);
    void setTrackState(int trackNumber, DiscMapWidget::TrackState state);
    void setPercent(int percent); // of the whole rip, re-reads included
    void addErrorMark(); // at the current read position
    // A pass over the disc after the audio (the CD+G sub-channel): a read line
    // and a thin band up to it, the tracks keep their colors. The caption goes
    // in front of the percentage. trackNumber < 0 ends the pass.
    void setScanPosition(int trackNumber, qint64 sectors, const QString &caption = QString());

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Track {
        qint64 sectors = 0;
        int number = 0;
        TrackState state = TrackState::Pending;
        qint64 read = 0; // sectors of the running (or last) extraction
    };

    int indexOf(int trackNumber) const;
    qint64 offsetOf(int index) const; // first sector of the track on the map
    qint64 readPosition() const; // -1 if nothing is being read

    QList<Track> m_tracks;
    QList<qint64> m_errors; // sector offsets of read problems
    qint64 m_totalSectors = 0;
    int m_current = -1; // index of the track being read
    int m_percent = 0;
    qint64 m_scan = -1; // position of the pass after the audio, -1: none
    QString m_scanCaption;
};
