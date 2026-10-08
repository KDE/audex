/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_schemewizardwidgetUI.h"

#include "dialogs/textviewdialog.h"

#include <QDialog>
#include <QPointer>

class SchemeWizardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SchemeWizardDialog(const QString &scheme, QWidget *parent = nullptr);
    ~SchemeWizardDialog() override;

    QString scheme;

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
    void insSuffix();
    void insNoOfTracks();

    void update_example();

    void slotAccepted();
    void slotApplied();

private:
    Ui::SchemeWizardWidgetUI ui;

    bool save();

    QPushButton *applyButton = nullptr;

    QPointer<TextViewDialog> help_dialog;
};
