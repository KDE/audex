/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WAVEWIDGET_H
#define WAVEWIDGET_H

#include <QDir>
#include <QWidget>

#include "utils/encoderassistant.h"
#include "utils/error.h"
#include "utils/parameters.h"

#include "ui_wavewidgetUI.h"

class waveWidgetUI : public QWidget, public Ui::WAVEWidgetUI
{
public:
    explicit waveWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class waveWidget : public waveWidgetUI
{
    Q_OBJECT
public:
    explicit waveWidget(Parameters *parameters, QWidget *parent = nullptr);
    ~waveWidget() override;
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
    void trigger_changed();

private:
    Parameters *parameters;
    Error error;
    bool changed;
};

#endif
