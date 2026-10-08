/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_profiledatahookwizardwidgetUI.h"

#include <QDialog>

class ProfileDataHookWizardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDataHookWizardDialog(const QString &command, QWidget *parent = nullptr);

    QString command;

private Q_SLOTS:
    void insAlbumArtist();
    void insAlbumTitle();
    void insDate();
    void insGenre();
    void insNoOfTracks();
    void insToday();
    void insOutputDir();
    void insFileList();

    void update_example();
    void slotAccepted();

private:
    Ui::ProfileDataHookWizardWidgetUI ui;

    void insert(const QString &variable);
};
