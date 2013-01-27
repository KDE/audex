/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2013 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
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

#include "coverbrowserdialog.h"
#include "utils/errordialog.h"

CoverBrowserDialog::CoverBrowserDialog(QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  setup();
  showButtonSeparator(TRUE);

}

CoverBrowserDialog::~CoverBrowserDialog() {

}

void CoverBrowserDialog::fetchThumbnails(const QString& searchstring, const int fetchCount) {
  if (fetchCount == 0)
    cover_fetcher.startFetchThumbnails(searchstring, Preferences::fetchCount());
  else
    cover_fetcher.startFetchThumbnails(searchstring, fetchCount);
  ui.label->setText(i18n("Searching for covers..."));
}

void CoverBrowserDialog::startFetchCover(const int no) {
  cover_fetcher.startFetchCover(no);
}

void CoverBrowserDialog::slotButtonClicked(int button) {
  if (button == KDialog::Ok)
    select_this(ui.listWidget->selectedItems().at(0));
  else
    KDialog::slotButtonClicked(button);
}

void CoverBrowserDialog::select_this(QListWidgetItem* item) {
  cover_fetcher.stopFetchThumbnails();
  int match = item->data(Qt::UserRole).toInt();
  cover_fetcher.startFetchCover(match);
  accept();
}

void CoverBrowserDialog::enable_select_button() {
  enableButtonOk(ui.listWidget->selectedItems().count() > 0);
}

void CoverBrowserDialog::add_item(const QByteArray& cover, const QString& caption, int no) {
  QListWidgetItem *item = new QListWidgetItem;
  QPixmap pixmap;
  if (pixmap.loadFromData(cover)) {
    item->setText(caption);
    //item->setToolTip(i18n("%1\nCover Size: %2x%3", caption, pixmap.width(), pixmap.height()));
    item->setIcon(QIcon(pixmap.scaled(128, 128, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
    item->setData(Qt::UserRole, no-1);
    ui.listWidget->addItem(item);
  }
  ui.label->setText(i18n("Fetching Thumbnail %1 / %2...", no, cover_fetcher.count()));
}

void CoverBrowserDialog::all_fetched() {
  ui.label->setText(i18np("Found 1 Cover", "Found %1 Covers", cover_fetcher.count()));
  emit allCoverThumbnailsFetched();
}

void CoverBrowserDialog::nothing_fetched() {
  ui.label->setText(i18n("No Covers Found"));
}

void CoverBrowserDialog::cover_fetched(const QByteArray& cover) {
  emit coverFetched(cover);
}

void CoverBrowserDialog::error(const QString& description, const QString& solution) {
  ErrorDialog::show(this, description, solution);
}

void CoverBrowserDialog::setup() {

  static const int constIconSize=128;

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  setCaption(i18n("Fetch Cover From Google"));
  setButtons(KDialog::Ok | KDialog::Cancel);

  connect(&cover_fetcher, SIGNAL(fetchedThumbnail(const QByteArray&, const QString&, int)), this, SLOT(add_item(const QByteArray&, const QString&, int)));
  connect(&cover_fetcher, SIGNAL(allCoverThumbnailsFetched()), this, SLOT(all_fetched()));
  connect(&cover_fetcher, SIGNAL(nothingFetched()), this, SLOT(nothing_fetched()));
  connect(&cover_fetcher, SIGNAL(fetchedCover(const QByteArray&)), this, SLOT(cover_fetched(const QByteArray&)));
  connect(&cover_fetcher, SIGNAL(error(const QString&, const QString&)), this, SLOT(error(const QString&, const QString&)));
  
  ui.listWidget->setIconSize(QSize(constIconSize, constIconSize));
  ui.listWidget->setWordWrap(TRUE);
  ui.listWidget->setViewMode(QListView::IconMode);
  connect(ui.listWidget, SIGNAL(itemSelectionChanged()), this, SLOT(enable_select_button()));
  connect(ui.listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(select_this(QListWidgetItem*)));
  ui.listWidget->setMinimumSize((constIconSize+12)*4, (constIconSize+12)*2);
  enable_select_button();

}
