/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SIMPLEPATTERNWIZARDDIALOG_H
#define SIMPLEPATTERNWIZARDDIALOG_H

#include <QWhatsThis>
#include <QWidget>

#include <QDialog>

#include "utils/patternparser.h"

#include "ui_simplepatternwizardwidgetUI.h"

class SimplePatternWizardDialog : public QDialog
{
    Q_OBJECT

public:
    SimplePatternWizardDialog(const QString &pattern, const QString &suffix, QWidget *parent = nullptr);
    ~SimplePatternWizardDialog() override;

    QString pattern;

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
    Ui::SimplePatternWizardWidgetUI ui;
    QString suffix;
    QPushButton *applyButton;

    bool save();
};

#endif
