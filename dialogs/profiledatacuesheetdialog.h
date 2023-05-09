/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEDATACUESHEETDIALOG_H
#define PROFILEDATACUESHEETDIALOG_H

#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "simplepatternwizarddialog.h"

#include "ui_profiledatacuesheetwidgetUI.h"

class ProfileDataCueSheetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDataCueSheetDialog(const QString &pattern, QWidget *parent = nullptr);
    ~ProfileDataCueSheetDialog() override;

    QString pattern;

protected Q_SLOTS:
    void pattern_wizard();

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
