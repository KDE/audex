/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "logviewdialog.h"

#include "dialogs/errordialog.h"

#include <KLocalizedString>

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QSaveFile>
#include <QTextStream>
#include <QVBoxLayout>

LogViewDialog::LogViewDialog(const QStringList &log, const QString &title, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    setWindowTitle(title);

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &LogViewDialog::slotSaveLog);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &LogViewDialog::slotClosed);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    ui.ktextedit->setPlainText(log.join("\n"));

    this->log = log;
}

void LogViewDialog::slotClosed()
{
    close();
}

void LogViewDialog::slotSaveLog()
{
    save();
}

void LogViewDialog::save()
{
    const QString fileName = QFileDialog::getSaveFileName(this, i18nc("@title:window", "Save Log"), QDir::homePath(), "*.log");
    if (fileName.isEmpty())
        return;

    // as shown: both the rip log and the check report name program and date
    QSaveFile data(fileName);
    if (data.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream out(&data);
        for (int i = 0; i < log.count(); i++)
            out << log.at(i) << Qt::endl;
        out.flush();
    }
    if (data.error() != QFileDevice::NoError || !data.commit())
        ErrorDialog::show(this, i18n("The log could not be saved to %1.", fileName), data.errorString(), i18nc("@title:window", "Save Log"));
}
