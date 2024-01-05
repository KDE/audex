/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROTOCOLVIEWDIALOG_H
#define PROTOCOLVIEWDIALOG_H

#include <QDateTime>
#include <QWidget>

#include <QDialog>

#include "ui_protocolviewwidgetUI.h"

class ProtocolViewDialog : public QDialog
{
    Q_OBJECT

public:
    ProtocolViewDialog(const QStringList &protocol, const QString &title, QWidget *parent = nullptr);
    ~ProtocolViewDialog() override;

private Q_SLOTS:
    void slotSaveProtocol();
    void slotClosed();
    void save();

private:
    Ui::ProtocolViewWidgetUI ui;

    QStringList protocol;
    QString title;
};

#endif
