/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_opusencwidgetUI.h"

#include <QWidget>

#include "utils/error.h"
#include "utils/parameters.h"

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
    explicit opusencWidget(const Parameters &parameters, QWidget *parent = nullptr);
    ~opusencWidget() override;
    inline const Parameters &parameters() const
    {
        return p_parameters;
    }
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
    Parameters p_parameters;
    Error error;
    bool changed;
};
