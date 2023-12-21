/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CUSTOMWIDGET_H
#define CUSTOMWIDGET_H

#include <QDir>
#include <QWidget>

#include "utils/encoderassistant.h"
#include "utils/error.h"
#include "utils/parameters.h"

#include "dialogs/commandwizarddialog.h"

#include "ui_customwidgetUI.h"

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
    void pattern_wizard();
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
