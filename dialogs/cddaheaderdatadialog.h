/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_cddaheaderdatawidgetUI.h"

#include "models/cdinfomodel.h"

#include <QDialog>
#include <QPointer>
#include <QPushButton>

#include <optional>

class CDDAHeaderDataDialog : public QDialog
{
    Q_OBJECT

public:
    // hdcd: result of the HDCD detection, shown read-only (empty: not checked)
    CDDAHeaderDataDialog(Audex::CDInfoModel *cddaModel, std::optional<bool> hdcd, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    void save();
    void trigger_changed();
    void enable_checkbox_multicd(bool enabled);

    void slotAccepted();
    void slotApplied();

private:
    Ui::CDDAHeaderDataWidgetUI ui;
    QPointer<Audex::CDInfoModel> cdda_model;
    QPushButton *okButton = nullptr;
    QPushButton *applyButton = nullptr;
};
