/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LAMEWIDGET_H
#define LAMEWIDGET_H

#include <QList>
#include <QWidget>

#include "utils/encoderassistant.h"
#include "utils/error.h"
#include "utils/parameters.h"

#include "ui_lamewidgetUI.h"

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
    explicit lameWidget(Parameters *parameters, QWidget *parent = nullptr);
    ~lameWidget() override;
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
    Parameters *parameters;
    Error error;
    bool changed;
    bool p_cbr_flag;
    QList<int> bitrates;
    int real_bitrate;
    int preset;
};

#endif
