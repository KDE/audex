/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_flacwidgetUI.h"

#include <QWidget>

#include "utils/error.h"
#include "utils/parameters.h"

class flacWidgetUI : public QWidget, public Ui::FLACWidgetUI
{
public:
    explicit flacWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class flacWidget : public flacWidgetUI
{
    Q_OBJECT
public:
    explicit flacWidget(const Parameters &parameters, QWidget *parent = nullptr);
    ~flacWidget() override;
    inline const Parameters &parameters() const
    {
        return p_parameters;
    }
    inline Error lastError() const
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
    void compression_changed_by_slider(int compression);
    void compression_changed_by_spinbox(int compression);
    void trigger_changed();

private:
    Parameters p_parameters;
    Error error;
    bool changed;
};
