/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LOGVIEWDIALOG_H
#define LOGVIEWDIALOG_H

#include <QDateTime>
#include <QWidget>

#include <QDialog>

#include "ui_logviewwidgetUI.h"

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

#endif
