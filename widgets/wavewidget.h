/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDir>
#include <QWidget>

#include "datatypes/error.h"
#include "utils/encoderassistant.h"
#include "utils/parameters.h"

#include "ui_wavewidgetUI.h"

namespace Audex
{

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

}
