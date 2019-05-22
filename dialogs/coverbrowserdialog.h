/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <http://userbase.kde.org/Audex>
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

#include <QPixmap>
#include <QIcon>
#include <QByteArray>

#include <QDialog>
#include <QPushButton>

#include "preferences.h"

#include "utils/coverfetcher.h"
#include "dialogs/errordialog.h"

#include "ui_coverbrowserwidgetUI.h"

class CoverBrowserDialog : public QDialog {

  Q_OBJECT

public:
  CoverBrowserDialog(QWidget *parent = 0);
  ~CoverBrowserDialog();

  inline int count() { return cover_fetcher.count(); }

public slots:
  void fetchThumbnails(const QString& searchstring, const int fetchCount = 0);
  void startFetchCover(const int no);

signals:
  void coverFetched(const QByteArray& cover);
  void allCoverThumbnailsFetched();
  void nothingFetched();

private slots:
  void select_this(QListWidgetItem* item);

  void enable_select_button();

  void add_item(const QByteArray& cover, const QString& caption, int no);
  void all_fetched();
  void nothing_fetched();

  void cover_fetched(const QByteArray& cover);

  void error(const QString& description, const QString& solution);

  void slotAccepted();

private:
  Ui::CoverBrowserWidgetUI ui;
  CoverFetcher cover_fetcher;
  QPushButton *okButton;

  void setup();

};

#endif
