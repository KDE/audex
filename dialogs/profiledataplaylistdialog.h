/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEDATAPLAYLISTDIALOG_H
#define PROFILEDATAPLAYLISTDIALOG_H

#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "simpleschemewizarddialog.h"

#include "ui_profiledataplaylistwidgetUI.h"

class ProfileDataPlaylistDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataPlaylistDialog(const QString &format, const QString &scheme, const bool absFilePath, const bool utf8, QWidget *parent = nullptr);
    ~ProfileDataPlaylistDialog() override;

    QString format;
    QString scheme;
    bool absFilePath;
    bool utf8;

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();
    void enable_abs_file_path(bool enabled);
    void enable_utf8(bool enabled);

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataPlaylistWidgetUI ui;
    QPushButton *applyButton;

    bool save();
};

#endif
