/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_filenameschemewizardwidgetUI.h"

#include "dialogs/textviewdialog.h"

#include <QDialog>
#include <QPointer>

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
    QPushButton *applyButton = nullptr;

    bool save();

    QPointer<TextViewDialog> help_dialog;
};
