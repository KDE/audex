/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FAACWIDGET_H
#define FAACWIDGET_H

#include <QDir>
#include <QWidget>

#include "utils/encoderassistant.h"
#include "utils/error.h"
#include "utils/parameters.h"

#include "ui_faacwidgetUI.h"

class faacWidgetUI : public QWidget, public Ui::FAACWidgetUI
{
public:
    explicit faacWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class faacWidget : public faacWidgetUI
{
    Q_OBJECT
public:
    explicit faacWidget(Parameters *parameters, QWidget *parent = nullptr);
    ~faacWidget() override;
    Error lastError() const
    {
        return error;
    }
    inline bool isChanged() const
    {
        return changed;
    }
public Q_SLOTS:
    bool save();
Q_SIGNALS:
    void triggerChanged();
private Q_SLOTS:
    void quality_changed_by_slider(int quality);
    void quality_changed_by_spinbox(int quality);
    void trigger_changed();

private:
    Parameters *parameters;
    Error error;
    bool changed;
};

#endif
