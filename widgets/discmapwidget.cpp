/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "discmapwidget.h"

#include <KColorScheme>
#include <KLocalizedString>

#include <QFontMetricsF>
#include <QPainter>

#include <algorithm>

namespace
{
constexpr int kMargin = 6;
constexpr int kBarHeight = 20;
constexpr int kLabelHeight = 16;
}

DiscMapWidget::DiscMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip(
        i18n("Blue: read · Green: confirmed by AccurateRip or the CUETools database · Yellow: not confirmed, read again "
             "in secure mode · Red: could not be read reliably"));
}

QSize DiscMapWidget::sizeHint() const
{
    return {400, 2 * kMargin + kBarHeight + kLabelHeight};
}

QSize DiscMapWidget::minimumSizeHint() const
{
    return {200, 2 * kMargin + kBarHeight + kLabelHeight};
}

void DiscMapWidget::setTracks(const QList<QPair<qint64, int>> &tracks)
{
    m_tracks.clear();
    m_totalSectors = 0;
    for (const auto &t : tracks) {
        m_tracks.append(Track{t.first, t.second});
        m_totalSectors += t.first;
    }
    m_errors.clear();
    m_current = -1;
    m_percent = 0;
    update();
}

void DiscMapWidget::setTrackProgress(int trackNumber, qint64 sectors)
{
    const int i = indexOf(trackNumber);
    if (i < 0)
        return;
    Track &t = m_tracks[i];
    m_current = i;
    t.read = std::clamp<qint64>(sectors, 0, t.sectors);
    if (t.state == TrackState::Pending)
        t.state = TrackState::Reading;
    update();
}

void DiscMapWidget::setTrackState(int trackNumber, TrackState state)
{
    const int i = indexOf(trackNumber);
    if (i < 0)
        return;
    Track &t = m_tracks[i];
    t.state = state;
    if (state == TrackState::Reading || state == TrackState::Rereading) {
        t.read = 0;
        m_current = i;
    } else {
        t.read = t.sectors;
    }
    update();
}

void DiscMapWidget::setPercent(int percent)
{
    m_percent = std::clamp(percent, 0, 100);
    update();
}

void DiscMapWidget::addErrorMark()
{
    const qint64 position = readPosition();
    if (position < 0)
        return;
    // re-reads of the same sector would otherwise stack identical ticks
    if (!m_errors.isEmpty() && m_errors.constLast() == position)
        return;
    m_errors.append(position);
    update();
}

int DiscMapWidget::indexOf(int trackNumber) const
{
    for (int i = 0; i < m_tracks.size(); ++i)
        if (m_tracks.at(i).number == trackNumber)
            return i;
    return -1;
}

qint64 DiscMapWidget::offsetOf(int index) const
{
    qint64 offset = 0;
    for (int i = 0; i < index; ++i)
        offset += m_tracks.at(i).sectors;
    return offset;
}

qint64 DiscMapWidget::readPosition() const
{
    if (m_current < 0)
        return -1;
    return offsetOf(m_current) + m_tracks.at(m_current).read;
}

void DiscMapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF bar(kMargin, kMargin, width() - 2.0 * kMargin, kBarHeight);

    p.setPen(Qt::NoPen);
    p.setBrush(palette().color(QPalette::AlternateBase));
    p.drawRoundedRect(bar, 3, 3);

    if (m_totalSectors <= 0)
        return;

    const auto xOf = [&bar, this](qint64 sector) {
        return bar.left() + (double(sector) / double(m_totalSectors)) * bar.width();
    };

    const KColorScheme scheme(QPalette::Active, KColorScheme::View);
    const QColor read = palette().color(QPalette::Accent);
    const QColor confirmed = scheme.foreground(KColorScheme::PositiveText).color();
    const QColor unconfirmed = scheme.foreground(KColorScheme::NeutralText).color();
    const QColor failed = scheme.foreground(KColorScheme::NegativeText).color();

    // tracks, colored by their state
    p.setClipRect(bar);
    qint64 offset = 0;
    for (const Track &t : std::as_const(m_tracks)) {
        const double left = xOf(offset);
        const double right = xOf(offset + t.sectors);
        const double readTo = xOf(offset + t.read);
        offset += t.sectors;
        const auto fill = [&](double to, const QColor &color) {
            p.fillRect(QRectF(QPointF(left, bar.top()), QPointF(to, bar.bottom())), color);
        };
        switch (t.state) {
        case TrackState::Pending:
            break;
        case TrackState::Reading:
            fill(readTo, read);
            break;
        case TrackState::Rereading:
            fill(right, unconfirmed);
            fill(readTo, read);
            break;
        case TrackState::Confirmed:
            fill(right, confirmed);
            break;
        case TrackState::Unconfirmed:
            fill(right, unconfirmed);
            break;
        case TrackState::Done:
            fill(right, read);
            break;
        case TrackState::Suspicious:
            fill(right, failed);
            break;
        }
    }
    p.setClipping(false);

    // read position
    const qint64 position = readPosition();
    const bool reading = m_current >= 0 && (m_tracks.at(m_current).state == TrackState::Reading || m_tracks.at(m_current).state == TrackState::Rereading);
    if (reading) {
        const double posX = xOf(position);
        p.setPen(QPen(palette().color(QPalette::WindowText), 2));
        p.drawLine(QPointF(posX, bar.top()), QPointF(posX, bar.bottom()));
    }

    // sectors with read problems
    p.setPen(QPen(failed, 1));
    for (const qint64 sector : std::as_const(m_errors)) {
        const double x = xOf(sector);
        p.drawLine(QPointF(x, bar.top() - 2), QPointF(x, bar.bottom() + 2));
    }

    // track boundaries and numbers
    const QFontMetricsF fm(p.font());
    offset = 0;
    for (const Track &t : std::as_const(m_tracks)) {
        const double left = xOf(offset);
        offset += t.sectors;
        const double right = xOf(offset);

        if (offset < m_totalSectors) {
            p.setPen(QPen(palette().color(QPalette::Mid), 1));
            p.drawLine(QPointF(right, bar.top() - 1), QPointF(right, bar.bottom() + 1));
        }

        const QString label = QString::number(t.number);
        const double w = fm.horizontalAdvance(label);
        if (right - left > w + 6) {
            p.setPen(palette().color(QPalette::WindowText));
            p.drawText(QPointF(left + (right - left - w) / 2.0, bar.bottom() + 2 + fm.ascent()), label);
        }
    }

    // percentage on a small plate: readable on every track color. Hidden
    // while tracks wait for or go through a secure re-read: the total grows
    // with every discarded track, so the number would jump back.
    const bool rereads = std::any_of(m_tracks.cbegin(), m_tracks.cend(), [](const Track &t) {
        return t.state == TrackState::Unconfirmed || t.state == TrackState::Rereading;
    });
    if (!rereads) {
        const QString percent = QStringLiteral("%1%").arg(m_percent);
        const QSizeF textSize(fm.horizontalAdvance(percent) + 8, fm.height());
        const QRectF plate(bar.center() - QPointF(textSize.width() / 2.0, textSize.height() / 2.0), textSize);
        QColor plateColor = palette().color(QPalette::Window);
        plateColor.setAlpha(200);
        p.setPen(Qt::NoPen);
        p.setBrush(plateColor);
        p.drawRoundedRect(plate, 3, 3);
        p.setPen(palette().color(QPalette::WindowText));
        p.drawText(plate, Qt::AlignCenter, percent);
    }

    // outline
    p.setPen(QPen(palette().color(QPalette::Mid), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(bar, 3, 3);
}
