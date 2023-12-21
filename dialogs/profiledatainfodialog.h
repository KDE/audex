/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEDATAINFODIALOG_H
#define PROFILEDATAINFODIALOG_H

#include <QWidget>

#include <QDialog>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

#include "simplepatternwizarddialog.h"

#include "ui_profiledatainfowidgetUI.h"

class ProfileDataInfoDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataInfoDialog(const QStringList &text, const QString &pattern, const QString &suffix, QWidget *parent = nullptr);
    ~ProfileDataInfoDialog() override;

    QStringList text;
    QString pattern;
    QString suffix;

protected Q_SLOTS:
    void pattern_wizard();

private Q_SLOTS:
    void trigger_changed();

    void about_variables();

    void load_text();
    void save_text();

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataInfoWidgetUI ui;
    QVBoxLayout *mainLayout;
    QPushButton *applyButton;

    bool save();
};

#endif
