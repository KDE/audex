/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEDATASINGLEFILEDIALOG_H
#define PROFILEDATASINGLEFILEDIALOG_H

#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "filenameschemewizarddialog.h"

#include "ui_profiledatasinglefilewidgetUI.h"

class ProfileDataSingleFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileDataSingleFileDialog(const QString &scheme, QWidget *parent = nullptr);
    ~ProfileDataSingleFileDialog() override;

    QString scheme;

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataSingleFileWidgetUI ui;
    QPushButton *applyButton;

    bool save();
};

#endif
