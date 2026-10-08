/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_logviewwidgetUI.h"

#include <QDialog>

class LogViewDialog : public QDialog
{
    Q_OBJECT

public:
    LogViewDialog(const QStringList &log, const QString &title, QWidget *parent = nullptr);

private Q_SLOTS:
    void slotSaveLog();
    void slotClosed();
    void save();

private:
    Ui::LogViewWidgetUI ui;

    QStringList log;
    QString title;
};
