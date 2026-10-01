/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_wavewidgetUI.h"

#include <QWidget>

#include "utils/error.h"
#include "utils/parameters.h"

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
    explicit waveWidget(const Parameters &parameters, QWidget *parent = nullptr);
    ~waveWidget() override;
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
    void trigger_changed();

private:
    Parameters p_parameters;
    Error error;
    bool changed;
};
