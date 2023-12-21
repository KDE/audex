/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CDDAHEADERDATADIALOG_H
#define CDDAHEADERDATADIALOG_H

#include <QDate>
#include <QDialog>
#include <QPushButton>
#include <QWidget>

#include "models/cddamodel.h"

#include "utils/discidcalculator.h"

#include "ui_cddaheaderdatawidgetUI.h"

class CDDAHeaderDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CDDAHeaderDataDialog(CDDAModel *cddaModel, QWidget *parent = nullptr);
    ~CDDAHeaderDataDialog() override;

private Q_SLOTS:
    void save();
    void trigger_changed();
    void enable_checkbox_multicd(bool enabled);

    void slotAccepted();
    void slotApplied();

private:
    Ui::CDDAHeaderDataWidgetUI ui;
    CDDAModel *cdda_model;
    QPushButton *okButton;
    QPushButton *applyButton;
};

#endif
