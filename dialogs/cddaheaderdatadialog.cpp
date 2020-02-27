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

#include "cddaheaderdatadialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

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

CDDAHeaderDataDialog::CDDAHeaderDataDialog(CDDAModel *cddaModel, QWidget *parent) : QDialog(parent) {

  Q_UNUSED(parent);

  cdda_model = cddaModel;
  if (!cdda_model) {
    qDebug() << "CDDAModel is NULL!";
    return;
  }



  setWindowTitle(i18n("Edit Data"));

  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Apply|QDialogButtonBox::Cancel);
  okButton = buttonBox->button(QDialogButtonBox::Ok);
  applyButton = buttonBox->button(QDialogButtonBox::Apply);
  okButton->setDefault(true);
  okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &CDDAHeaderDataDialog::slotAccepted);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &CDDAHeaderDataDialog::reject);
  connect(applyButton, &QPushButton::clicked, this, &CDDAHeaderDataDialog::slotApplied);

  QWidget *widget = new QWidget(this);
  mainLayout->addWidget(widget);
  mainLayout->addWidget(buttonBox);
  ui.setupUi(widget);

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
  ui.qlineedit_artist->setText(cdda_model->artist());
  connect(ui.qlineedit_artist, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));
  ui.qlineedit_title->setText(cdda_model->title());
  connect(ui.qlineedit_title, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));
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
  ui.qlineedit_freedbdiscid->setText(QString("0x%1").arg(DiscIDCalculator::FreeDBId(cdda_model->discSignature()), 0, 16));

  enable_checkbox_multicd(cdda_model->isMultiCD());

  applyButton->setEnabled(false);

}

CDDAHeaderDataDialog::~CDDAHeaderDataDialog() {

}

void CDDAHeaderDataDialog::slotAccepted() {
    save();
    accept();
}

void CDDAHeaderDataDialog::slotApplied() {
    save();
}

void CDDAHeaderDataDialog::save() {

  cdda_model->setVarious(ui.checkBox_various->isChecked());
  cdda_model->setMultiCD(ui.checkBox_multicd->isChecked());
  cdda_model->setArtist(ui.qlineedit_artist->text());
  cdda_model->setTitle(ui.qlineedit_title->text());
  cdda_model->setCDNum(ui.kintspinbox_cdnum->value());
  cdda_model->setTrackOffset(ui.kintspinbox_trackoffset->value());
  cdda_model->setGenre(ui.kcombobox_genre->lineEdit()->text());
  cdda_model->setYear(QString("%1").arg(ui.kintspinbox_year->value()));
  cdda_model->setExtendedData(ui.ktextedit_extdata->toPlainText().split('\n'));
  applyButton->setEnabled(false);

}

void CDDAHeaderDataDialog::trigger_changed() {

  if (ui.checkBox_various->isChecked() != cdda_model->isVarious()) { applyButton->setEnabled(true); return; }
  if (ui.checkBox_multicd->isChecked() != cdda_model->isMultiCD()) { applyButton->setEnabled(true); return; }
  if (ui.qlineedit_artist->text() != cdda_model->artist()) { applyButton->setEnabled(true); return; }
  if (ui.qlineedit_title->text() != cdda_model->title()) { applyButton->setEnabled(true); return; }
  if (ui.checkBox_various->isChecked())
    if (ui.kintspinbox_cdnum->value() != cdda_model->cdNum()) { applyButton->setEnabled(true); return; }
  if (ui.kintspinbox_trackoffset->value() != cdda_model->trackOffset()) { applyButton->setEnabled(true); return; }
  if (ui.kcombobox_genre->lineEdit()->text() != cdda_model->genre()) { applyButton->setEnabled(true); return; }
  if (ui.kintspinbox_year->value() != cdda_model->year().toInt()) { applyButton->setEnabled(true); return; }
  if (ui.ktextedit_extdata->toPlainText().split('\n') != cdda_model->extendedData()) { applyButton->setEnabled(true); return; }

  applyButton->setEnabled(false);

}

void CDDAHeaderDataDialog::enable_checkbox_multicd(bool enabled) {

  ui.kintspinbox_cdnum->setEnabled(enabled);
  ui.label_cdnum->setEnabled(enabled);

}
