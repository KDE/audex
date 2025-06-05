/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QWidget>

#include <QDialog>

#include "dialogs/textviewdialog.h"
#include "utils/schemeparser.h"

#include "ui_filenameschemewizardwidgetUI.h"

namespace Audex
{

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
    QPointer<QPushButton> applyButton;

    bool save();

    QPointer<TextViewDialog> help_dialog;
};

}
