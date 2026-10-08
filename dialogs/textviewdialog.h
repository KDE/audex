/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_textviewwidgetUI.h"

#include <QDialog>

class TextViewDialog : public QDialog
{
    Q_OBJECT

public:
    TextViewDialog(const QString &text = QString(), const QString &title = QString(), QWidget *parent = nullptr);

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
