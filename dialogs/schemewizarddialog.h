/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "dialogs/textviewdialog.h"
#include "utils/schemeparser.h"

#include "ui_schemewizardwidgetUI.h"

namespace Audex
{

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

    QPointer<QPushButton> applyButton;

    QPointer<TextViewDialog> help_dialog;
};

}
