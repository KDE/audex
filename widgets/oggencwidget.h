/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OGGENCWIDGET_H
#define OGGENCWIDGET_H

#include <QDir>
#include <QWidget>

#include "utils/encoderassistant.h"
#include "utils/error.h"
#include "utils/parameters.h"

#include "ui_oggencwidgetUI.h"

class oggencWidgetUI : public QWidget, public Ui::OggEncWidgetUI
{
public:
    explicit oggencWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class oggencWidget : public oggencWidgetUI
{
    Q_OBJECT
public:
    explicit oggencWidget(Parameters *parameters, QWidget *parent = nullptr);
    ~oggencWidget() override;
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
    void quality_changed_by_slider(int quality);
    void quality_changed_by_spinbox(double quality);
    void set_bitrate(float quality);
    void enable_min_bitrate(bool enable);
    void set_minbitrate(int bitrate);
    void enable_max_bitrate(bool enable);
    void set_maxbitrate(int bitrate);
    void trigger_changed();

private:
    Parameters *parameters;
    Error error;
    bool changed;
};

#endif
