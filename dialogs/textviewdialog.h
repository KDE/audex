/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QWidget>

#include <QDialog>

#include "ui_textviewwidgetUI.h"

namespace Audex
{

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

}
