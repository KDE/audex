/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SCHEMEWIZARDDIALOG_H
#define SCHEMEWIZARDDIALOG_H

#include <QWhatsThis>
#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "utils/schemeparser.h"

#include "ui_schemewizardwidgetUI.h"

class SchemeWizardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SchemeWizardDialog(const QString &scheme, QWidget *parent = nullptr);
    ~SchemeWizardDialog() override;

    QString scheme;

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
    Ui::SchemeWizardWidgetUI ui;

    bool save();

    QPushButton *applyButton;
};

#endif
