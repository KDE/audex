/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "errordialog.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace ErrorDialog
{
void show(QWidget *parent, const QString &message, const QString &details, const QString &caption, Error::ErrorType type)
{
    const bool warning = type == Error::WARNING;
    const QString title = !caption.isEmpty() ? caption : warning ? i18n("Warning") : i18n("Error");
    if (details.trimmed().isEmpty()) {
        KMessageBox::error(parent, message, title);
        return;
    }

    QDialog dialog(parent);
    dialog.setWindowTitle(title);

    auto *iconLabel = new QLabel(&dialog);
    iconLabel->setPixmap(dialog.style()->standardIcon(warning ? QStyle::SP_MessageBoxWarning : QStyle::SP_MessageBoxCritical).pixmap(48, 48));
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

    auto *detailsBox = new QPlainTextEdit(&dialog);
    detailsBox->setPlainText(details);
    detailsBox->setReadOnly(true);
    detailsBox->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    detailsBox->setMinimumSize(560, 180);
    layout->addWidget(detailsBox);

    // in the top right corner of the box, above the text
    const QIcon copyIcon = QIcon::fromTheme(QStringLiteral("edit-copy"));
    auto *copyButton = new QToolButton(detailsBox->viewport());
    copyButton->setIcon(copyIcon);
    copyButton->setText(i18n("Copy")); // shown without an icon theme
    copyButton->setAutoRaise(true);
    copyButton->setToolTip(i18n("Copy to clipboard"));
    copyButton->setCursor(Qt::ArrowCursor);
    auto *overlay = new QGridLayout(detailsBox->viewport());
    overlay->setContentsMargins(0, 0, 0, 0);
    overlay->addWidget(copyButton, 0, 0, Qt::AlignTop | Qt::AlignRight);
    QObject::connect(copyButton, &QToolButton::clicked, copyButton, [copyButton, copyIcon, details]() {
        QGuiApplication::clipboard()->setText(details);
        copyButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
        copyButton->setText(i18n("Copied"));
        QTimer::singleShot(1500, copyButton, [copyButton, copyIcon]() {
            copyButton->setIcon(copyIcon);
            copyButton->setText(i18n("Copy"));
        });
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    dialog.exec();
}

void show(QWidget *parent, const Error &error, const QString &caption)
{
    QString text = error.isValid() ? error.message() : i18n("Unknown error.");
    if (!error.details().isEmpty())
        text += QStringLiteral("\n\n") + error.details();
    const QString title = !caption.isEmpty() ? caption : error.errorType() == Error::WARNING ? i18n("Warning") : i18n("Error");
    KMessageBox::error(parent, text, title);
}

}
