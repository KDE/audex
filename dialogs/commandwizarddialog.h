/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_commandwizardwidgetUI.h"

#include <QDialog>
#include <QPointer>
#include <QPushButton>

#include "dialogs/textviewdialog.h"

class CommandWizardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommandWizardDialog(const QString &command, const QString &suffix, QWidget *parent = nullptr);
    ~CommandWizardDialog() override;

    QString command;

private Q_SLOTS:
    void trigger_changed();

    void help();

    void insAlbumArtist();
    void insAlbumTitle();
    void insTrackArtist();
    void insTrackTitle();
    void insTrackNo();
    void insCDNo();
    void insDate();
    void insGenre();
    void insNoOfTracks();
    void insInFile();
    void insOutFile();

    void update_example();

    void slotAccepted();
    void slotApplied();

private:
    Ui::CommandWizardWidgetUI ui;

    QString m_suffix;

    bool save();

    QPushButton *okButton = nullptr;
    QPushButton *applyButton = nullptr;

    QPointer<TextViewDialog> help_dialog;
};
