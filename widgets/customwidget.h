/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDir>
#include <QWidget>

#include "datatypes//error.h"
#include "utils/encoderassistant.h"
#include "utils/parameters.h"

#include "dialogs/commandwizarddialog.h"

#include "ui_customwidgetUI.h"

namespace Audex
{

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
    explicit customWidget(Parameters *parameters, QWidget *parent = nullptr);
    ~customWidget() override;
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
    Parameters *parameters;
    Error error;
    bool changed;
};

}
