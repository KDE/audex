/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROTOCOLDIALOG_H
#define PROTOCOLDIALOG_H

#include <QDateTime>
#include <QWidget>

#include <QDialog>

#include "ui_protocolwidgetUI.h"

class ProtocolDialog : public QDialog
{
    Q_OBJECT

public:
    ProtocolDialog(const QStringList &protocol, const QString &title, QWidget *parent = nullptr);
    ~ProtocolDialog() override;

private Q_SLOTS:
    void slotSaveProtocol();
    void slotClosed();
    void save();

private:
    Ui::ProtocolWidgetUI ui;

    QStringList protocol;
    QString title;
};

#endif
