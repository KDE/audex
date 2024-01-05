/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "textviewdialog.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

TextViewDialog::TextViewDialog(const QString &text, const QString &title, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    if (!title.isEmpty())
        setWindowTitle(title);

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &TextViewDialog::slotClosed);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    if (!text.isEmpty())
        ui.ktextedit->setText(text);
}

TextViewDialog::~TextViewDialog()
{
}

void TextViewDialog::slotClosed()
{
    hide();
}
