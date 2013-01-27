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

#include "cddaheaderdatadialog.h"

#define GENRE_MAX 148
static const char *ID3_GENRES[GENRE_MAX] = {
  "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop",
  "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae",
  "Rock", "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack",
  "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical",
  "Instrumental", "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "Alt",
  "Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop",
  "Instrumental Rock", "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic",
  "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult",
  "Gangsta Rap", "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American",
  "Cabaret", "New Wave", "Psychedelic", "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal",
  "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock", "Folk",
  "Folk/Rock", "National Folk", "Swing", "Fast-Fusion", "Bebob", "Latin", "Revival",
  "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock", "Progressive Rock",
  "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band", "Chorus", "Easy Listening",
  "Acoustic", "Humour", "Speech", "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony",
  "Booty Bass", "Primus", "Porn Groove", "Satire", "Slow Jam", "Club", "Tango",
  "Samba", "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
  "Punk Rock", "Drum Solo", "A Cappella", "Euro-House", "Dance Hall", "Goa",
  "Drum & Bass", "Club-House", "Hardcore", "Terror", "Indie", "BritPop", "Negerpunk",
  "Polsk Punk", "Beat", "Christian Gangsta Rap", "Heavy Metal", "Black Metal", "Crossover",
  "Contemporary Christian", "Christian Rock", "Merengue", "Salsa", "Thrash Metal",
  "Anime", "JPop", "Synthpop"
};

CDDAHeaderDataDialog::CDDAHeaderDataDialog(CDDAModel *cddaModel, QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  cdda_model = cddaModel;
  if (!cdda_model) {
    kDebug() << "CDDAModel is NULL!";
    return;
  }

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  setCaption(i18n("Edit Data"));
  setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

  QStringList genres; for (int i = 0; i < GENRE_MAX; ++i) genres.append(QString().fromAscii(ID3_GENRES[i]));
  genres.sort();
  KCompletion *comp = ui.kcombobox_genre->completionObject();
  comp->insertItems(genres);
  ui.kcombobox_genre->addItems(genres);
  connect(ui.kcombobox_genre, SIGNAL(returnPressed(const QString&)), comp, SLOT(addItem(const QString&)));

  ui.checkBox_various->setChecked(cdda_model->isVarious());
  connect(ui.checkBox_various, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));
  ui.checkBox_multicd->setChecked(cdda_model->isMultiCD());
  connect(ui.checkBox_multicd, SIGNAL(toggled(bool)), this, SLOT(enable_checkbox_multicd(bool)));
  connect(ui.checkBox_multicd, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));
  ui.klineedit_artist->setText(cdda_model->artist());
  connect(ui.klineedit_artist, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));
  ui.klineedit_title->setText(cdda_model->title());
  connect(ui.klineedit_title, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));
  ui.kintspinbox_cdnum->setValue(cdda_model->cdNum());
  connect(ui.kintspinbox_cdnum, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));
  ui.kintspinbox_trackoffset->setValue(cdda_model->trackOffset());
  connect(ui.kintspinbox_trackoffset, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));
  ui.kcombobox_genre->lineEdit()->setText(cdda_model->genre());
  connect(ui.kcombobox_genre->lineEdit(), SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));
  {
    bool ok; int year = cdda_model->year().toInt(&ok);
    if (ok) ui.kintspinbox_year->setValue(year); else ui.kintspinbox_year->setValue(QDate::currentDate().year());
  }
  connect(ui.kintspinbox_year, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));
  ui.ktextedit_extdata->setText(cdda_model->extendedData().join("\n"));
  connect(ui.ktextedit_extdata, SIGNAL(textChanged()), this, SLOT(trigger_changed()));
  ui.label_discid_2->setText(QString("0x%1").arg(cdda_model->discid(), 0, 16));

  enable_checkbox_multicd(cdda_model->isMultiCD());

  enableButtonApply(FALSE);
  showButtonSeparator(true);

}

CDDAHeaderDataDialog::~CDDAHeaderDataDialog() {

}

void CDDAHeaderDataDialog::slotButtonClicked(int button) {
  if (button == KDialog::Ok) {
    save();
    accept();
  } else if (button == KDialog::Apply) {
    save();
  } else {
    KDialog::slotButtonClicked(button);
  }
}

void CDDAHeaderDataDialog::save() {

  cdda_model->setVarious(ui.checkBox_various->isChecked());
  cdda_model->setMultiCD(ui.checkBox_multicd->isChecked());
  cdda_model->setArtist(ui.klineedit_artist->text());
  cdda_model->setTitle(ui.klineedit_title->text());
  cdda_model->setCDNum(ui.kintspinbox_cdnum->value());
  cdda_model->setTrackOffset(ui.kintspinbox_trackoffset->value());
  cdda_model->setGenre(ui.kcombobox_genre->lineEdit()->text());
  cdda_model->setYear(QString("%1").arg(ui.kintspinbox_year->value()));
  cdda_model->setExtendedData(ui.ktextedit_extdata->toPlainText().split("\n"));
  enableButtonApply(FALSE);

}

void CDDAHeaderDataDialog::trigger_changed() {

  if (ui.checkBox_various->isChecked() != cdda_model->isVarious()) { enableButtonApply(TRUE); return; }
  if (ui.checkBox_multicd->isChecked() != cdda_model->isMultiCD()) { enableButtonApply(TRUE); return; }
  if (ui.klineedit_artist->text() != cdda_model->artist()) { enableButtonApply(TRUE); return; }
  if (ui.klineedit_title->text() != cdda_model->title()) { enableButtonApply(TRUE); return; }
  if (ui.checkBox_various->isChecked())
    if (ui.kintspinbox_cdnum->value() != cdda_model->cdNum()) { enableButtonApply(TRUE); return; }
  if (ui.kintspinbox_trackoffset->value() != cdda_model->trackOffset()) { enableButtonApply(TRUE); return; }
  if (ui.kcombobox_genre->lineEdit()->text() != cdda_model->genre()) { enableButtonApply(TRUE); return; }
  if (ui.kintspinbox_year->value() != cdda_model->year().toInt()) { enableButtonApply(TRUE); return; }
  if (ui.ktextedit_extdata->toPlainText().split("\n") != cdda_model->extendedData()) { enableButtonApply(TRUE); return; }

  enableButtonApply(FALSE);

}

void CDDAHeaderDataDialog::enable_checkbox_multicd(bool enabled) {

  ui.kintspinbox_cdnum->setEnabled(enabled);
  ui.label_cdnum->setEnabled(enabled);

}
