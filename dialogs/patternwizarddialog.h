/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PATTERNWIZARDDIALOG_H
#define PATTERNWIZARDDIALOG_H

#include <QWhatsThis>
#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "utils/patternparser.h"

#include "ui_patternwizardwidgetUI.h"

class PatternWizardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PatternWizardDialog(const QString &pattern, QWidget *parent = nullptr);
    ~PatternWizardDialog() override;

    QString pattern;

private Q_SLOTS:
    void trigger_changed();

    void about_filename_schemes();
    void about_parameters();

    void insAlbumArtist();
    void insAlbumTitle();
    void insTrackArtist();
    void insTrackTitle();
    void insTrackNo();
    void insCDNo();
    void insDate();
    void insGenre();
    void insSuffix();
    void insNoOfTracks();

    void update_example();

    void slotAccepted();
    void slotApplied();

private:
    Ui::PatternWizardWidgetUI ui;

    bool save();

    QPushButton *applyButton;
};

#endif
