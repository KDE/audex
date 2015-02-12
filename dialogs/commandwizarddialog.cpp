/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2014 Marco Nelles (audex@maniatek.com)
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

#include "commandwizarddialog.h"

CommandWizardDialog::CommandWizardDialog(const QString& command, QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  setCaption(i18n("Command Pattern Wizard"));

  setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

  ui.klineedit_command->setText(command);
  connect(ui.klineedit_command, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));
  connect(ui.klineedit_command, SIGNAL(textChanged(const QString&)), this, SLOT(update_example()));
  ui.klineedit_command->setCursorPosition(0);

  connect(ui.kurllabel_aboutcommandlineschemes, SIGNAL(leftClickedUrl()), this, SLOT(about_commandline_schemes()));
  connect(ui.kurllabel_aboutparameters, SIGNAL(leftClickedUrl()), this, SLOT(about_parameters()));

  connect(ui.kpushbutton_albumartist, SIGNAL(clicked()), this, SLOT(insAlbumArtist()));
  connect(ui.kpushbutton_albumtitle, SIGNAL(clicked()), this, SLOT(insAlbumTitle()));
  connect(ui.kpushbutton_trackartist, SIGNAL(clicked()), this, SLOT(insTrackArtist()));
  connect(ui.kpushbutton_tracktitle, SIGNAL(clicked()), this, SLOT(insTrackTitle()));
  connect(ui.kpushbutton_trackno, SIGNAL(clicked()), this, SLOT(insTrackNo()));
  connect(ui.kpushbutton_cdno, SIGNAL(clicked()), this, SLOT(insCDNo()));
  connect(ui.kpushbutton_date, SIGNAL(clicked()), this, SLOT(insDate()));
  connect(ui.kpushbutton_genre, SIGNAL(clicked()), this, SLOT(insGenre()));
  connect(ui.kpushbutton_cover_file, SIGNAL(clicked()), this, SLOT(insCoverFile()));
  connect(ui.kpushbutton_nooftracks, SIGNAL(clicked()), this, SLOT(insNoOfTracks()));
  connect(ui.kpushbutton_input_file, SIGNAL(clicked()), this, SLOT(insInFile()));
  connect(ui.kpushbutton_output_file, SIGNAL(clicked()), this, SLOT(insOutFile()));

  this->command = command;

  enableButtonApply(FALSE);
  showButtonSeparator(true);

  update_example();

}

CommandWizardDialog::~CommandWizardDialog() {

}

void CommandWizardDialog::slotButtonClicked(int button) {
  if (button == KDialog::Ok) {
    save();
    accept();
  } else if (button == KDialog::Apply) {
    save();
  } else {
    KDialog::slotButtonClicked(button);
  }
}

void CommandWizardDialog::trigger_changed() {
  if (ui.klineedit_command->text() != command) { enableButtonApply(TRUE); return; }
  enableButtonApply(FALSE);
}

void CommandWizardDialog::about_commandline_schemes() {
  QWhatsThis::showText(ui.kurllabel_aboutcommandlineschemes->mapToGlobal(ui.kurllabel_aboutcommandlineschemes->geometry().topLeft()),
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
                               "<tr><td>$cover</td><td>The cover file.</td></tr>"
                               "<tr><td>$nooftracks</td><td>The total number of audio tracks of the CD.</td></tr>"
                               "<tr><td>$i</td><td>The temporary WAV file (input file) created by Audex from CD audio track. You can use it as a normal input file for your command line encoder.</td></tr>"
                               "<tr><td>$o</td><td>The full output filename and path (output file). Use it as the output for your command line encoder.</td></tr>"
                               "<tr><td>$encoder</td><td>Encoder name and version.</td></tr>"
                               "<tr><td>$audex</td><td>Audex name and version.</td></tr>"
                               "</table></p>"),
                          ui.kurllabel_aboutcommandlineschemes);
}

void CommandWizardDialog::about_parameters() {
  QWhatsThis::showText(ui.kurllabel_aboutparameters->mapToGlobal(ui.kurllabel_aboutparameters->geometry().topLeft()),
                          i18n("<p>Variables in Audex can have parameters. E.g.</p>"
                               "<pre>${cover format=\"JPG\" x=\"300\" y=\"300\" preparam=\"-ti \"}</pre>"
                               "<p>A filename of a JPG image with the size of 300x300 will be inserted. "
                               "If no size is set, the size of the original cover file will be taken. If "
                               "no cover is set, this variable will be replaced by nothing, otherwise "
                               "something like <i>/tmp/cover.123.jpg</i> will be inserted. Possible "
                               "formats are \"JPG\", \"PNG\" and \"GIF\" (Default: \"JPG\").</p>\n"
                               "<p><i><b>Note:</b> LAME discards cover files larger than 128 KiB. Please "
                               "bear this in mind, if you set the format and size.</i></p>"
                               "<hr />"
                               "<p>\"preparam\" and \"postparam\" define parameters inserted before "
                               "(pre) or behind (post) the variable. These values are <b>only</b> shown if a value is set."
                               "Works with all commandline variables.</p>"
                               "<hr />"
                               "<p><b><i>To have a complete overview of parameters go to the Audex webpage.</i></b></p>"
                               ),
                          ui.kurllabel_aboutparameters);
}

void CommandWizardDialog::insAlbumArtist() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_ALBUM_ARTIST));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insAlbumTitle() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_ALBUM_TITLE));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insTrackArtist() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_TRACK_ARTIST));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insTrackTitle() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_TRACK_TITLE));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insTrackNo() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_TRACK_NO));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insCDNo() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_CD_NO));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insDate() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_DATE));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insGenre() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_GENRE));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insCoverFile() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_COVER_FILE));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insNoOfTracks() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), QString("$"VAR_NO_OF_TRACKS));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insInFile() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_INPUT_FILE));
  ui.klineedit_command->setText(text);
  update_example();
}

void CommandWizardDialog::insOutFile() {
  QString text = ui.klineedit_command->text();
  text.insert(ui.klineedit_command->cursorPosition(), "$"+QString(VAR_OUTPUT_FILE));
  ui.klineedit_command->setText(text);
  update_example();
}

bool CommandWizardDialog::save() {
  command = ui.klineedit_command->text();
  enableButtonApply(FALSE);
  return TRUE;
}

void CommandWizardDialog::update_example() {
  PatternParser patternparser;
  QString filename = patternparser.parseCommandPattern(ui.klineedit_command->text(),
        "/tmp/tmp.wav", QString("%1/music/Meat Loaf/02 - Meat Loaf - Blind As A Bat.ogg").arg(QDir::homePath()),
        2, 1, 1, 12,
        "Meat Loaf", "Bat Out Of Hell III", "Meat Loaf", "Blind As A Bat",
        "2006", "Rock", "ogg", QImage(), FALSE, QDir::tempPath(), "LAME 3.98.2", TRUE);
  ui.klineedit_album_example->setText(filename);
  ui.klineedit_album_example->setCursorPosition(0);
  filename = patternparser.parseCommandPattern(ui.klineedit_command->text(),
        "/tmp/tmp.wav", QString("%1/music/Alternative Hits/Volume 4/04 - Wolfsheim - Approaching Lightspeed.ogg").arg(QDir::homePath()),
        4, 2, 1, 18,
        "Alternative Hits", "Volume 4", "Wolfsheim", "Approaching Lightspeed",
        "2003", "Darkwave", "ogg", QImage(), FALSE, QDir::tempPath(), "LAME 3.98.2", TRUE);
  ui.klineedit_sampler_example->setText(filename);
  ui.klineedit_sampler_example->setCursorPosition(0);
}
