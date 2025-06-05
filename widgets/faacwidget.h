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

#include "ui_faacwidgetUI.h"

namespace Audex
{

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

}
