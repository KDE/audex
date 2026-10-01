/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "errordialog.h"

#include <KLocalizedString>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QStyle>
#include <QVBoxLayout>

namespace ErrorDialog
{
void show(QWidget *parent, const QString &message, const QString &details, const QString &caption)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(caption.isEmpty() ? i18n("Error") : caption);

    auto *iconLabel = new QLabel(&dialog);
    iconLabel->setPixmap(dialog.style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(48, 48));
    iconLabel->setAlignment(Qt::AlignTop);

    auto *messageLabel = new QLabel(message, &dialog);
    QFont bold = messageLabel->font();
    bold.setBold(true);
    messageLabel->setFont(bold);
    messageLabel->setWordWrap(true);
    messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *topLayout = new QHBoxLayout;
    topLayout->addWidget(iconLabel);
    topLayout->addWidget(messageLabel, 1);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addLayout(topLayout);

    if (!details.isEmpty()) {
        auto *detailsBox = new QPlainTextEdit(&dialog);
        detailsBox->setPlainText(details);
        detailsBox->setReadOnly(true);
        detailsBox->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        detailsBox->setMinimumSize(560, 180);
        layout->addWidget(detailsBox);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    dialog.exec();
}

}
