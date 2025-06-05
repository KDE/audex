/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "textviewdialog.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace Audex
{

TextViewDialog::TextViewDialog(const QString &text, const QString &title, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    if (!title.isEmpty())
        setWindowTitle(title);

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &TextViewDialog::slotClosed);

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

}
