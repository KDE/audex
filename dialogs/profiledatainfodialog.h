/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    ProfileDataInfoDialog(const QStringList &text, const QString &pattern, const QString &suffix, QWidget *parent = 0);
    ~ProfileDataInfoDialog() override;

    QStringList text;
    QString pattern;
    QString suffix;

protected slots:
    void pattern_wizard();

private slots:
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
