/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDateTime>
#include <QWidget>

#include <QDialog>

#include "ui_logviewwidgetUI.h"

namespace Audex
{

class LogViewDialog : public QDialog
{
    Q_OBJECT

public:
    LogViewDialog(const QStringList &log, const QString &title, QWidget *parent = nullptr);
    ~LogViewDialog() override;

private Q_SLOTS:
    void slotSaveLog();
    void slotClosed();
    void save();

private:
    Ui::LogViewWidgetUI ui;

    QStringList log;
    QString title;
};

}
