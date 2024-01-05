/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FILENAMESCHEMEWIZARDDIALOG_H
#define FILENAMESCHEMEWIZARDDIALOG_H

#include <QWidget>

#include <QDialog>

#include "dialogs/textviewdialog.h"
#include "utils/schemeparser.h"

#include "ui_filenameschemewizardwidgetUI.h"

class FilenameSchemeWizardDialog : public QDialog
{
    Q_OBJECT

public:
    FilenameSchemeWizardDialog(const QString &scheme, const QString &suffix, QWidget *parent = nullptr);
    ~FilenameSchemeWizardDialog() override;

    QString scheme;

private Q_SLOTS:
    void trigger_changed();

    void help();

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
    Ui::FilenameSchemeWizardWidgetUI ui;
    QString suffix;
    QPushButton *applyButton;

    bool save();

    TextViewDialog *help_dialog;
};

#endif
