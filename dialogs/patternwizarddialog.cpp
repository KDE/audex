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

#include "patternwizarddialog.h"

PatternWizardDialog::PatternWizardDialog(const QString& pattern, QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);
  
  setCaption(i18n("Filename Pattern Wizard"));

  setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

  ui.klineedit_pattern->setText(pattern);
  connect(ui.klineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));
  connect(ui.klineedit_pattern, SIGNAL(textChanged(const QString&)), this, SLOT(update_example()));
  ui.klineedit_pattern->setCursorPosition(0);

  connect(ui.kurllabel_aboutfilenameschemes, SIGNAL(leftClickedUrl()), this, SLOT(about_filename_schemes()));
  connect(ui.kurllabel_aboutparameters, SIGNAL(leftClickedUrl()), this, SLOT(about_parameters()));
  
  connect(ui.kpushbutton_albumartist, SIGNAL(clicked()), this, SLOT(insAlbumArtist()));
  connect(ui.kpushbutton_albumtitle, SIGNAL(clicked()), this, SLOT(insAlbumTitle()));
  connect(ui.kpushbutton_trackartist, SIGNAL(clicked()), this, SLOT(insTrackArtist()));
  connect(ui.kpushbutton_tracktitle, SIGNAL(clicked()), this, SLOT(insTrackTitle()));
  connect(ui.kpushbutton_trackno, SIGNAL(clicked()), this, SLOT(insTrackNo()));
  connect(ui.kpushbutton_cdno, SIGNAL(clicked()), this, SLOT(insCDNo()));
  connect(ui.kpushbutton_date, SIGNAL(clicked()), this, SLOT(insDate()));
  connect(ui.kpushbutton_genre, SIGNAL(clicked()), this, SLOT(insGenre()));
  connect(ui.kpushbutton_suffix, SIGNAL(clicked()), this, SLOT(insSuffix()));
  connect(ui.kpushbutton_nooftracks, SIGNAL(clicked()), this, SLOT(insNoOfTracks()));

  this->pattern = pattern;

  enableButtonApply(false);
  showButtonSeparator(true);

  update_example();

}

PatternWizardDialog::~PatternWizardDialog() {

}

void PatternWizardDialog::slotButtonClicked(int button) {
  if (button == KDialog::Ok) {
    save();
    accept();
  } else if (button == KDialog::Apply) {
    save();
  } else {
    KDialog::slotButtonClicked(button);
  }
}

void PatternWizardDialog::trigger_changed() {
  if (ui.klineedit_pattern->text() != pattern) { enableButtonApply(true); return; }
  enableButtonApply(false);
}

void PatternWizardDialog::about_filename_schemes() {
  QWhatsThis::showText(ui.kurllabel_aboutfilenameschemes->mapToGlobal(ui.kurllabel_aboutfilenameschemes->geometry().topLeft()),
                          i18n("<p>The following variables will be replaced with their particular meaning in every track name.</p>"
                               "<p><table border=\"1\">"
                               "<tr><th><em>Variable</em></th><th><em>Description</em></th></tr>"
                               "<tr><td>$artist</td><td>The artist of the CD. If your CD is a compilation, then this tag represents the title in most cases.</td></tr>"
                               "<tr><td>$title</td><td>The title of the CD. If your CD is a compilation, then this tag represents the subtitle in most cases.</td></tr>"
                               "<tr><td>$date</td><td>The release date of the CD. In almost all cases this is the year.</td></tr>"
                               "<tr><td>$genre</td><td>The genre of the CD.</td></tr>"
                               "<tr><td>$cdno</td><td>The CD number of a multi-CD album. Often compilations consist of several CDs. <i>Note:</i> If the multi-CD flag is <b>not</b> set for the current CD, than this value will be just empty.</td></tr>"
                               "<tr><td>$tartist</td><td>This is the artist of every individual track. It is especially useful on compilation CDs.</td></tr>"
                               "<tr><td>$ttitle</td><td>The track title. Normally each track on a CD has its own title, which is the name of the song.</td></tr>"
                               "<tr><td>$trackno</td><td>The track number. First track is 1.</td></tr>"
                               "<tr><td>$nooftracks</td><td>The total number of audio tracks of the CD.</td></tr>"
                               "<tr><td>$suffix</td><td>The filename extension.</td></tr>"
                               "<tr><td>$audex</td><td>Audex name and version.</td></tr>"
                               "</table></p>"),
                          ui.kurllabel_aboutfilenameschemes);
}

void PatternWizardDialog::about_parameters() {
  QWhatsThis::showText(ui.kurllabel_aboutparameters->mapToGlobal(ui.kurllabel_aboutparameters->geometry().topLeft()),
                          i18n("<p>Variables in Audex can have parameters. E.g.</p>"
                               "<pre>$artist/$title/${trackno length=\"2\" fillchar=\"0\"} - $ttile.$suffix</pre>"
                               "<p>This means, that the tracknumber will be forced to a length of 2 digits. If the number is less than 10, it will be filled up with \"0\".</p>"
                               "<pre>$artist/${title lowercase=\"true\"}/$trackno - $ttile.$suffix<br />"
                               "$artist/${title uppercase=\"true\"}/$trackno - $ttile.$suffix</pre>"
                               "<p>The title will be uppercased or lowercased. This works with the other variables, too.</p>"
                               "<pre>${artist left=\"1\"}/$artist/$title/$trackno - $ttile.$suffix</pre>"
                               "<p>Take one character from the left, which is the first one.</p>"
                               "<hr />"
                               "<p><b><i>To have a complete overview of parameters go to the Audex webpage.</i></b></p>"
                               ),
                          ui.kurllabel_aboutparameters);
}

void PatternWizardDialog::insAlbumArtist() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_ALBUM_ARTIST));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void PatternWizardDialog::insAlbumTitle() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_ALBUM_TITLE));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void PatternWizardDialog::insTrackArtist() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_TRACK_ARTIST));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void PatternWizardDialog::insTrackTitle() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_TRACK_TITLE));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void PatternWizardDialog::insTrackNo() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_TRACK_NO));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void PatternWizardDialog::insCDNo() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_CD_NO));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void PatternWizardDialog::insDate() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_DATE));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void PatternWizardDialog::insGenre() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_GENRE));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void PatternWizardDialog::insSuffix() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_SUFFIX));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void PatternWizardDialog::insNoOfTracks() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_NO_OF_TRACKS));
  ui.klineedit_pattern->setText(text);
  update_example();
}

bool PatternWizardDialog::save() {
  pattern = ui.klineedit_pattern->text();
  enableButtonApply(false);
  return true;
}

void PatternWizardDialog::update_example() {
  PatternParser patternparser;
  QString filename = patternparser.parseFilenamePattern(ui.klineedit_pattern->text(),
	2, 1, 12, 1,
	"Meat Loaf", "Bat Out Of Hell III", "Meat Loaf", "Blind As A Bat",
	"2006", "Rock", "ogg", false, false, false);
  ui.klineedit_album_example->setText(filename);
  ui.klineedit_album_example->setCursorPosition(0);
  filename = patternparser.parseFilenamePattern(ui.klineedit_pattern->text(),
	4, 2, 18, 1,
	"Alternative Hits", "Volume 4", "Wolfsheim", "Kein Zurueck",
	"2003", "Darkwave", "ogg", false, false, false);
  ui.klineedit_sampler_example->setText(filename);
  ui.klineedit_sampler_example->setCursorPosition(0);
}
