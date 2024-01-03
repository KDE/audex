/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SIMPLESCHEMEWIZARDDIALOG_H
#define SIMPLESCHEMEWIZARDDIALOG_H

#include <QWhatsThis>
#include <QWidget>

#include <QDialog>

#include "utils/schemeparser.h"

#include "ui_simpleschemewizardwidgetUI.h"

class SimpleSchemeWizardDialog : public QDialog
{
    Q_OBJECT

public:
    SimpleSchemeWizardDialog(const QString &scheme, const QString &suffix, QWidget *parent = nullptr);
    ~SimpleSchemeWizardDialog() override;

    QString scheme;

private Q_SLOTS:
    void trigger_changed();

    void about_schemes();
    void about_parameters();

    void insAlbumArtist();
    void insAlbumTitle();
    void insCDNo();
    void insDate();
    void insGenre();
    void insSuffix();
    void insNoOfTracks();

    void update_example();

    void slotAccepted();
    void slotApplied();

private:
    Ui::SimpleSchemeWizardWidgetUI ui;
    QString suffix;
    QPushButton *applyButton;

    bool save();
};

#endif
