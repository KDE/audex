/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEDATACOVERDIALOG_H
#define PROFILEDATACOVERDIALOG_H

#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "filenameschemewizarddialog.h"

#include "ui_profiledatacoverwidgetUI.h"

class ProfileDataCoverDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataCoverDialog(const bool scale, const QSize &size, const QString &format, const QString &scheme, QWidget *parent = nullptr);
    ~ProfileDataCoverDialog() override;

    bool scale;
    QSize size;
    QString format;
    QString scheme;

protected Q_SLOTS:
    void scheme_wizard();

private Q_SLOTS:
    void trigger_changed();
    void enable_scale(bool enabled);

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataCoverWidgetUI ui;
    QPushButton *applyButton;

    bool save();
};

#endif
