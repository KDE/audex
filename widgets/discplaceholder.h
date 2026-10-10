/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

// Stands in for the track list while there is no audio CD to show: an icon,
// a title, a line of explanation and the buttons that help on.
class DiscPlaceholder : public QWidget
{
    Q_OBJECT

public:
    enum Button {
        NoButton = 0x0,
        OpenTray = 0x1,
        Eject = 0x2,
        Retry = 0x4,
        Details = 0x8
    };
    Q_DECLARE_FLAGS(Buttons, Button)

    explicit DiscPlaceholder(QWidget *parent = nullptr);

    void setMessage(const QString &iconName, const QString &title, const QString &text, Buttons buttons = NoButton);

Q_SIGNALS:
    void ejectClicked(); // OpenTray and Eject
    void retryClicked();
    void detailsClicked();

private:
    QLabel *m_icon;
    QLabel *m_title;
    QLabel *m_text;
    QPushButton *m_openTray;
    QPushButton *m_eject;
    QPushButton *m_retry;
    QPushButton *m_details;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DiscPlaceholder::Buttons)
