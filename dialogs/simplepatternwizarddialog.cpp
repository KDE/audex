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

#include "simplepatternwizarddialog.h"

SimplePatternWizardDialog::SimplePatternWizardDialog(const QString& pattern, const QString& suffix, QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  setCaption(i18n("Pattern Wizard"));

  setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

  ui.klineedit_pattern->setText(pattern);
  connect(ui.klineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));
  connect(ui.klineedit_pattern, SIGNAL(textChanged(const QString&)), this, SLOT(update_example()));
  ui.klineedit_pattern->setCursorPosition(0);
  
  connect(ui.kurllabel_aboutschemes, SIGNAL(leftClickedUrl()), this, SLOT(about_schemes()));
  connect(ui.kurllabel_aboutparameters, SIGNAL(leftClickedUrl()), this, SLOT(about_parameters()));

  connect(ui.kpushbutton_albumartist, SIGNAL(clicked()), this, SLOT(insAlbumArtist()));
  connect(ui.kpushbutton_albumtitle, SIGNAL(clicked()), this, SLOT(insAlbumTitle()));
  connect(ui.kpushbutton_cdno, SIGNAL(clicked()), this, SLOT(insCDNo()));
  connect(ui.kpushbutton_date, SIGNAL(clicked()), this, SLOT(insDate()));
  connect(ui.kpushbutton_genre, SIGNAL(clicked()), this, SLOT(insGenre()));
  connect(ui.kpushbutton_suffix, SIGNAL(clicked()), this, SLOT(insSuffix()));
  connect(ui.kpushbutton_nooftracks, SIGNAL(clicked()), this, SLOT(insNoOfTracks()));

  this->pattern = pattern;
  this->suffix = suffix;

  enableButtonApply(false);
  showButtonSeparator(true);
  update_example();

}

SimplePatternWizardDialog::~SimplePatternWizardDialog() {

}

void SimplePatternWizardDialog::slotButtonClicked(int button) {
  if (button == KDialog::Ok) {
    save();
    accept();
  } else if (button == KDialog::Apply) {
    save();
  } else {
    KDialog::slotButtonClicked(button);
  }
}

void SimplePatternWizardDialog::trigger_changed() {
  if (ui.klineedit_pattern->text() != pattern) { enableButtonApply(true); return; }
  enableButtonApply(false);
}

void SimplePatternWizardDialog::about_schemes() {
  QWhatsThis::showText(ui.kurllabel_aboutschemes->mapToGlobal(ui.kurllabel_aboutschemes->geometry().topLeft()),
                          i18n("<p>Filename schemes without an absolute path will be relative to the ripped music files.</p>"
			       "<p>The following variables will be replaced with their particular meaning in every track name.</p>"
                               "<p><table border=\"1\">"
                               "<tr><th><em>Variable</em></th><th><em>Description</em></th></tr>"
                               "<tr><td>$artist</td><td>The artist of the CD. If your CD is a compilation, then this tag represents the title in most cases.</td></tr>"
                               "<tr><td>$title</td><td>The title of the CD. If your CD is a compilation, then this tag represents the subtitle in most cases.</td></tr>"
                               "<tr><td>$date</td><td>The release date of the CD. In almost all cases this is the year.</td></tr>"
                               "<tr><td>$genre</td><td>The genre of the CD.</td></tr>"
                               "<tr><td>$cdno</td><td>The CD number of a multi-CD album. Often compilations consist of several CDs. <i>Note:</i> If the multi-CD flag is <b>not</b> set for the current CD, than this value will be just empty.</td></tr>"
                               "<tr><td>$suffix</td><td>The filename extension (e.g. \".jpg\", \".m3u\", \".md5\" ...).</td></tr>"
                               "<tr><td>$nooftracks</td><td>The total number of audio tracks of the CD.</td></tr>"
                               "<tr><td>$encoder</td><td>Encoder name and version.</td></tr>"
                               "<tr><td>$audex</td><td>Audex name and version.</td></tr>"
                               "</table></p>"),
                          ui.kurllabel_aboutschemes);
}

void SimplePatternWizardDialog::about_parameters() {
  QWhatsThis::showText(ui.kurllabel_aboutparameters->mapToGlobal(ui.kurllabel_aboutparameters->geometry().topLeft()),
                          i18n("<p>Variables in Audex can have parameters. E.g.</p>"
                               "<pre>$title (${cdno length=\"2\" fillchar=\"0\"}).$suffix</pre>"
                               "<p>This means, that the cd number will be forced to a length of 2 digits. If the number is less than 10, it will be filled up with \"0\".</p>"
                               "<pre>${title lowercase=\"true\"}.$suffix<br />"
                               "${title uppercase=\"true\"}.$suffix</pre>"
                               "<p>The title will be uppercased or lowercased. This works with the other variables, too.</p>"
                               "<pre>${artist left=\"1\"} - $title.$suffix</pre>"
                               "<p>Take one character from the left, which is the first one.</p>"
                               "<hr />"
                               "<p><b><i>To have a complete overview of parameters go to the Audex webpage.</i></b></p>"
                               ),
                          ui.kurllabel_aboutparameters);
}

void SimplePatternWizardDialog::insAlbumArtist() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_ALBUM_ARTIST));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void SimplePatternWizardDialog::insAlbumTitle() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_ALBUM_TITLE));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void SimplePatternWizardDialog::insCDNo() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_CD_NO));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void SimplePatternWizardDialog::insDate() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_DATE));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void SimplePatternWizardDialog::insGenre() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_GENRE));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void SimplePatternWizardDialog::insSuffix() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_SUFFIX));
  ui.klineedit_pattern->setText(text);
  update_example();
}

void SimplePatternWizardDialog::insNoOfTracks() {
  QString text = ui.klineedit_pattern->text();
  text.insert(ui.klineedit_pattern->cursorPosition(), QString("$" VAR_NO_OF_TRACKS));
  ui.klineedit_pattern->setText(text);
  update_example();
}

bool SimplePatternWizardDialog::save() {
  pattern = ui.klineedit_pattern->text();
  enableButtonApply(false);
  return true;
}

void SimplePatternWizardDialog::update_example() {
  PatternParser patternparser;
  QString filename = patternparser.parseSimplePattern(ui.klineedit_pattern->text(),
	1, 12, "Meat Loaf", "Bat Out Of Hell III", "2006", "Rock", suffix, false);
  ui.klineedit_example->setText(filename);
  ui.klineedit_example->setCursorPosition(0);
}
