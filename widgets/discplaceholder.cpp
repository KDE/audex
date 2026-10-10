/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "discplaceholder.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

DiscPlaceholder::DiscPlaceholder(QWidget *parent)
    : QWidget(parent)
    , m_icon(new QLabel(this))
    , m_title(new QLabel(this))
    , m_text(new QLabel(this))
    , m_openTray(new QPushButton(QIcon::fromTheme(QStringLiteral("media-eject")), i18n("Open Tray"), this))
    , m_eject(new QPushButton(QIcon::fromTheme(QStringLiteral("media-eject")), i18n("Eject"), this))
    , m_retry(new QPushButton(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Try Again"), this))
    , m_details(new QPushButton(QIcon::fromTheme(QStringLiteral("help-about")), i18n("Details"), this))
{
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base); // like the track list it stands in for

    m_icon->setAlignment(Qt::AlignCenter);

    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.35);
    m_title->setFont(titleFont);
    m_title->setAlignment(Qt::AlignCenter);
    m_title->setWordWrap(true);

    m_text->setAlignment(Qt::AlignCenter);
    m_text->setWordWrap(true);
    m_text->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    for (QPushButton *button : {m_retry, m_openTray, m_eject, m_details})
        buttons->addWidget(button);
    buttons->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 20, 40, 20); // long texts wrap before the edges
    layout->addStretch();
    layout->addWidget(m_icon);
    layout->addWidget(m_title);
    layout->addWidget(m_text);
    layout->addSpacing(style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing));
    layout->addLayout(buttons);
    layout->addStretch();

    connect(m_openTray, &QPushButton::clicked, this, &DiscPlaceholder::ejectClicked);
    connect(m_eject, &QPushButton::clicked, this, &DiscPlaceholder::ejectClicked);
    connect(m_retry, &QPushButton::clicked, this, &DiscPlaceholder::retryClicked);
    connect(m_details, &QPushButton::clicked, this, &DiscPlaceholder::detailsClicked);
}

void DiscPlaceholder::setMessage(const QString &iconName, const QString &title, const QString &text, Buttons buttons)
{
    m_icon->setPixmap(QIcon::fromTheme(iconName).pixmap(64, 64));
    m_title->setText(title);
    m_text->setText(text);
    m_text->setVisible(!text.isEmpty());
    m_openTray->setVisible(buttons & OpenTray);
    m_eject->setVisible(buttons & Eject);
    m_retry->setVisible(buttons & Retry);
    m_details->setVisible(buttons & Details);
}
