/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEDATAHASHLISTDIALOG_H
#define PROFILEDATAHASHLISTDIALOG_H

#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "filenameschemewizarddialog.h"

#include "ui_profiledatahashlistwidgetUI.h"

class ProfileDataHashlistDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataHashlistDialog(const QString &scheme, const QString &format, QWidget *parent = nullptr);
    ~ProfileDataHashlistDialog() override;

    QString format;
    QString scheme;

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataHashlistWidgetUI ui;
    QPushButton *applyButton;

    bool save();
};

#endif
