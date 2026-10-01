/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_profilewidgetUI.h"

#include <QPointer>
#include <QWidget>

class ProfileModel;

class profileWidgetUI : public QWidget, public Ui::ProfileWidgetUI
{
public:
    explicit profileWidgetUI(QWidget *parent)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

class profileWidget : public profileWidgetUI
{
    Q_OBJECT
public:
    explicit profileWidget(ProfileModel *profileModel, QWidget *parent = nullptr);
    ~profileWidget() override;
private Q_SLOTS:
    void p_update();
    void add_profile();
    void rem_profile();
    void mod_profile(const QModelIndex &index);
    void mod_profile();
    void copy_profile();
    void save_profiles();
    void load_profiles();

private:
    QPointer<ProfileModel> profile_model;
};
