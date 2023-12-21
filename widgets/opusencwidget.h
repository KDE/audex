/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPUSENCWIDGET_H
#define OPUSENCWIDGET_H

#include <QDir>
#include <QWidget>

#include "utils/encoderassistant.h"
#include "utils/error.h"
#include "utils/parameters.h"

#include "ui_opusencwidgetUI.h"

class opusencWidgetUI : public QWidget, public Ui::OpusEncWidgetUI
{
public:
    explicit opusencWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class opusencWidget : public opusencWidgetUI
{
    Q_OBJECT
public:
    explicit opusencWidget(Parameters *parameters, QWidget *parent = nullptr);
    ~opusencWidget() override;
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
    void bitrate_changed_by_slider(int quality);
    void bitrate_changed_by_spinbox(int quality);
    void trigger_changed();

private:
    Parameters *parameters;
    Error error;
    bool changed;
};

#endif
