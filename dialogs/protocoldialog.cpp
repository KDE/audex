/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocoldialog.h"

#include <QFileDialog>
#include <QTextStream>

#include <KConfigGroup>
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

ProtocolDialog::ProtocolDialog(const QStringList &protocol, const QString &title, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    setWindowTitle(title);

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProtocolDialog::slotSaveProtocol);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProtocolDialog::slotClosed);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    ui.ktextedit->setPlainText(protocol.join("\n"));

    this->protocol = protocol;
    this->title = title;
}

ProtocolDialog::~ProtocolDialog()
{
}

void ProtocolDialog::slotClosed()
{
    close();
}

void ProtocolDialog::slotSaveProtocol()
{
    save();
}

void ProtocolDialog::save()
{
    QString fileName = QFileDialog::getSaveFileName(this, i18n("Save %1", title), QDir::homePath(), "*.pro");
    if (!fileName.isEmpty()) {
        QFile data(fileName);
        if (data.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&data);
            out << "AUDEX " << title << "\n";
            out << tr("created on ") << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
            out << "\n";
            for (int i = 0; i < protocol.count(); i++) {
                out << protocol.at(i) << "\n";
            }
        }
    }
}
