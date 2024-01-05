/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TEXTVIEWDIALOG_H
#define TEXTVIEWDIALOG_H

#include <QWidget>

#include <QDialog>

#include "ui_textviewwidgetUI.h"

class TextViewDialog : public QDialog
{
    Q_OBJECT

public:
    TextViewDialog(const QString &text = QString(), const QString &title = QString(), QWidget *parent = nullptr);
    ~TextViewDialog() override;

public Q_SLOTS:
    void setTitle(const QString &title)
    {
        setWindowTitle(title);
    }
    void setText(const QString &text)
    {
        ui.ktextedit->setText(text);
    }

private Q_SLOTS:
    void slotClosed();

private:
    Ui::TextViewWidgetUI ui;
};

#endif
