/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef COMMANDWIZARDDIALOG_H
#define COMMANDWIZARDDIALOG_H

#include <QWhatsThis>
#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "utils/patternparser.h"

#include "ui_commandwizardwidgetUI.h"

class CommandWizardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommandWizardDialog(const QString &command, QWidget *parent = nullptr);
    ~CommandWizardDialog() override;

    QString command;

private Q_SLOTS:
    void trigger_changed();

    void about_commandline_schemes();
    void about_parameters();

    void insAlbumArtist();
    void insAlbumTitle();
    void insTrackArtist();
    void insTrackTitle();
    void insTrackNo();
    void insCDNo();
    void insDate();
    void insGenre();
    void insCoverFile();
    void insNoOfTracks();
    void insInFile();
    void insOutFile();

    void update_example();

    void slotAccepted();
    void slotApplied();

private:
    Ui::CommandWizardWidgetUI ui;

    bool save();

    QPushButton *okButton;
    QPushButton *applyButton;
};

#endif
