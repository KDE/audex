/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_lamewidgetUI.h"

#include <QList>
#include <QWidget>

#include "utils/error.h"
#include "utils/parameters.h"

class lameWidgetUI : public QWidget, public Ui::LAMEWidgetUI
{
public:
    explicit lameWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class lameWidget : public lameWidgetUI
{
    Q_OBJECT
public:
    explicit lameWidget(const Parameters &parameters, QWidget *parent = nullptr);
    ~lameWidget() override;
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
Q_SIGNALS:
    void triggerChanged();
public Q_SLOTS:
    bool save();
private Q_SLOTS:
    void enable_medium(bool enable);
    void enable_standard(bool enable);
    void enable_extreme(bool enable);
    void enable_insane(bool enable);
    void enable_custom(bool enable);
    void enable_CBR(bool enable);
    void bitrate_changed_by_slider(int bitrate);
    void bitrate_changed_by_spinbox(int bitrate);
    void trigger_changed();

private:
    Parameters p_parameters;
    Error error;
    bool changed;
    bool p_cbr_flag;
    QList<int> bitrates;
    int real_bitrate;
    int preset;
};
