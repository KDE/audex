/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QWidget>

#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSizePolicy>
#include <QTextBrowser>

#include "dialogs/textviewdialog.h"
#include "utils/schemeparser.h"

#include "ui_commandwizardwidgetUI.h"

namespace Audex
{

class CommandWizardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommandWizardDialog(const QString &command, QWidget *parent = nullptr);
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

    QPointer<QPushButton> okButton;
    QPointer<QPushButton> applyButton;

    QPointer<TextViewDialog> help_dialog;
};

}
