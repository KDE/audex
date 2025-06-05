/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDir>
#include <QWidget>

#include <KMessageBox>

#include "dialogs/profiledatadialog.h"
#include "models/profilemodel.h"

#include "ui_profilewidgetUI.h"

namespace Audex
{

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
    void mod_profile_index(const QModelIndex &index);
    void mod_profile();
    void copy_profile();
    void save_profiles();
    void load_profiles();
    void init_profiles();

private:
    ProfileModel *profile_model;
};

}
