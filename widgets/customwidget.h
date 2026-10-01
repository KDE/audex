/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_customwidgetUI.h"

#include <QWidget>

#include "utils/error.h"
#include "utils/parameters.h"

class customWidgetUI : public QWidget, public Ui::CustomWidgetUI
{
public:
    explicit customWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class customWidget : public customWidgetUI
{
    Q_OBJECT
public:
    explicit customWidget(const Parameters &parameters, QWidget *parent = nullptr);
    ~customWidget() override;
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
    void scheme_wizard();
Q_SIGNALS:
    void triggerChanged();
private Q_SLOTS:
    void trigger_changed();

private:
    Parameters p_parameters;
    Error error;
    bool changed;
};
