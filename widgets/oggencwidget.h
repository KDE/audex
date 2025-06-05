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

#include "ui_oggencwidgetUI.h"

namespace Audex
{

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

}
