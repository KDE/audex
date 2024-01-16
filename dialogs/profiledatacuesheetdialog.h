/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEDATACUESHEETDIALOG_H
#define PROFILEDATACUESHEETDIALOG_H

#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "filenameschemewizarddialog.h"

#include "ui_profiledatacuesheetwidgetUI.h"

class ProfileDataCueSheetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDataCueSheetDialog(const QString &scheme, const bool writeMCNAndISRC, QWidget *parent = nullptr);
    ~ProfileDataCueSheetDialog() override;

    QString scheme;
    bool writeMCNAndISRC;

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataCueSheetWidgetUI ui;
    QPushButton *applyButton;

    bool save();
};

#endif
