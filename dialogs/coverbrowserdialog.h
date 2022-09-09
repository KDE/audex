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

#ifndef COVERBROWSERDIALOG_H
#define COVERBROWSERDIALOG_H

#include <QByteArray>
#include <QIcon>
#include <QPixmap>

#include <QDialog>
#include <QPushButton>

#include "preferences.h"

#include "dialogs/errordialog.h"
#include "utils/coverfetcher.h"

#include "ui_coverbrowserwidgetUI.h"

class CoverBrowserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoverBrowserDialog(QWidget *parent = nullptr);
    ~CoverBrowserDialog() override;

    inline int count()
    {
        return cover_fetcher.count();
    }

public Q_SLOTS:
    void fetchThumbnails(const QString &searchstring, const int fetchCount = 0);
    void startFetchCover(const int no);

Q_SIGNALS:
    void coverFetched(const QByteArray &cover);
    void allCoverThumbnailsFetched();
    void nothingFetched();

private Q_SLOTS:
    void select_this(QListWidgetItem *item);

    void enable_select_button();

    void add_item(const QByteArray &cover, const QString &caption, int no);
    void all_fetched();
    void nothing_fetched();

    void cover_fetched(const QByteArray &cover);

    void error(const QString &description, const QString &solution);

    void slotAccepted();

private:
    Ui::CoverBrowserWidgetUI ui;
    CoverFetcher cover_fetcher;
    QPushButton *okButton;

    void setup();
};

#endif
