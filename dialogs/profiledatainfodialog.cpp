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

#include "profiledatainfodialog.h"

ProfileDataInfoDialog::ProfileDataInfoDialog(const QStringList& text, const QString& pattern, const QString& suffix, QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  this->text = text;
  this->pattern = pattern;
  this->suffix = suffix;

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  setCaption(i18n("Info Settings"));

  setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

  connect(ui.kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));
  ui.kpushbutton_pattern->setIcon(KIcon("tools-wizard"));

  ui.ktextedit_text->setPlainText(text.join("\n"));
  connect(ui.ktextedit_text, SIGNAL(textChanged()), this, SLOT(trigger_changed()));

  ui.klineedit_pattern->setText(pattern);
  connect(ui.klineedit_pattern, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  ui.klineedit_suffix->setText(suffix);
  connect(ui.klineedit_suffix, SIGNAL(textEdited(const QString&)), this, SLOT(trigger_changed()));

  ui.kpushbutton_load->setIcon(KIcon("document-open"));
  ui.kpushbutton_save->setIcon(KIcon("document-save"));

  connect(ui.kurllabel_aboutvariables, SIGNAL(leftClickedUrl()), this, SLOT(about_variables()));
  
  connect(ui.kpushbutton_load, SIGNAL(clicked()), this, SLOT(load_text()));
  connect(ui.kpushbutton_save, SIGNAL(clicked()), this, SLOT(save_text()));

  enableButtonApply(FALSE);
  showButtonSeparator(true);

}

ProfileDataInfoDialog::~ProfileDataInfoDialog() {
}

void ProfileDataInfoDialog::slotButtonClicked(int button) {
  if (button == KDialog::Ok) {
    save();
    accept();
  } else if (button == KDialog::Apply) {
    save();
  } else {
    KDialog::slotButtonClicked(button);
  }
}

void ProfileDataInfoDialog::pattern_wizard() {

  SimplePatternWizardDialog *dialog = new SimplePatternWizardDialog(ui.klineedit_pattern->text(), suffix, this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }

  ui.klineedit_pattern->setText(dialog->pattern);

  delete dialog;

  trigger_changed();

}

void ProfileDataInfoDialog::trigger_changed() {
  if (ui.ktextedit_text->toPlainText().split("\n") != text) { enableButtonApply(TRUE); return; }
  if (ui.klineedit_suffix->text() != suffix) { enableButtonApply(TRUE); return; }
  if (ui.klineedit_pattern->text() != pattern) { enableButtonApply(TRUE); return; }
  enableButtonApply(FALSE);
}

void ProfileDataInfoDialog::about_variables() {

   KDialog *dialog = new KDialog(this);
   dialog->resize(QSize(700, 480));
   dialog->setCaption(i18n("Usable Variables For Text Template"));
   dialog->setButtons(KDialog::Ok);
  
   KTextBrowser *tb = new KTextBrowser(dialog);
   tb->setHtml(i18n("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
   "<html>"
   "<head>"
     "<style type=\"text/css\">"
     "p, li { white-space: pre-wrap; }"
     "</style>"
   "</head>"
   "<body>"
     "Variables will be replaced by a special value and can even contain parameters.<br />"
     "For example the variable "
     "<div style=\"font-family:monospace; background: #b3c1d6; color: black\"><pre>"
     "$artist"
     "</pre></div>"
     "or the equivalent"
     "<div style=\"font-family:monospace; background: #b3c1d6; color: black\"><pre>"
     "${artist}"
     "</pre></div>"
     "will be replaced by the relevant artist of the cd. Variables may also have attribute, for example:"
     "<div style=\"font-family:monospace; background: #b3c1d6; color: black\"><pre>"
     "${today format=\"yyyy-MM-dd\"}"
     "</pre></div>"
     "This would print the current date. Setting the format will control how this is done. The example (above)"
     "would result int the date being printed as 2010-10-07 (if this was the current date). See below for more details.<br /><br />"
     "You can make use of the following variables:<br />"
     "<table border=1>"
     "<thead>"
     "<tr>"
     "<th>Variable</th><th>Parameter</th><th>Description</th><th>Example</th>"
     "</tr>"
     "</thead>"
     "<tbody>"
     "<tr>"
     "<td>$artist</td><td></td><td>Prints the relevant artist of the extracted cd.</td><td>${artist }</td>"
     "</tr>"
     "<tr>"
     "<td>$title</td><td></td><td>Prints the relevant title of the extracted cd.</td><td></td>"
     "</tr>"
     "<tr>"
     "<td>$date</td><td></td><td>Prints the relevant date (usually release year) of the extracted cd.</td><td></td>"
     "</tr>"
     "<tr>"
     "<td>$genre;</td><td></td><td>Prints the relevant genre of the extracted cd.</td><td></td>"
     "</tr>"
     "<tr>"
     "<td>$size</td><td>iec,precision</td><td>Prints the overall size of all extracted (compressed) music files (incl. the cover). The attribute iec can be one of the following: b, k, m, g. b means byte, k KiB, m MiB and g GiB. The attribute precision gives the number of decimal places. Default attributes are iec=\"m\" and precision=\"2\"</td><td>${size iec=\"k\" precision=\"2\"}</td>"
     "</tr>"
     "<tr>"
     "<td>$length</td><td></td><td>Prints the relevant overall length of all extracted tracks. The format is min:sec.</td><td></td>"
     "</tr>"
     "<tr>"
     "<td>$nooftracks</td><td></td><td>Prints the total number of extracted tracks.</td><td></td>"
     "</tr>"
     "<tr>"
     "<td>$discid</td><td>base</td><td>Prints the discid of the current cd. The attribute base is the base of the number. The default is 16 (hexadecimal).</td><td>${discid base=\"16\"}</td>"
     "</tr>"
     "<tr>"
     "<td>$today</td><td>format</td><td>Prints the current date. The attribute format specifies the output (*).</td><td>${today format=\"yyyy-MM-dddd\"}</td>"
     "</tr>"
     "<tr>"
     "<td>$now</td><td>format</td><td>Prints the current date and/or time. The attribute format specifies the output (*).</td><td>${now format=\"yyyy-MM-dddd hh:mm:ss\"}</td>"
     "</tr>"
     "<tr>"
     "<td>$encoder</td><td></td><td>Prints encoder name and version.</td>"
     "</tr>"
     "<tr>"
     "<td>$audex</td><td></td><td>Prints Audex name and version.</td>"
     "</tr>"
     "<tr>"
     "<td>$br</td><td></td><td>Prints a linebreak.</td><td></td>"
     "</tr>"
     "</tbody>"
     "</table>"
     "<br /><br />"
     "(* date/time format expressions)"
     "<table cellpadding=\"2\" cellspacing=\"1\" border=\"1\">"
     "<thead><tr valign=\"top\"><th>Expression</th><th>Output</th></tr></thead>"
     "<tr valign=\"top\"><td>d</td><td>The day as a number without a leading zero (1 to 31).</td></tr>"
     "<tr valign=\"top\"><td>dd</td><td>The day as a number with a leading zero (01 to 31).</td></tr>"
     "<tr valign=\"top\"><td>ddd</td><td>The abbreviated localized day name (e.g&#x2e; 'Mon' to 'Sun').</td></tr>"
     "<tr valign=\"top\"><td>dddd</td><td>The long localized day name (e.g&#x2e; 'Monday' to 'Sunday').</td></tr>"
     "<tr valign=\"top\"><td>M</td><td>The month as a number without a leading zero (1 to 12).</td></tr>"
     "<tr valign=\"top\"><td>MM</td><td>The month as a number with a leading zero (01 to 12).</td></tr>"
     "<tr valign=\"top\"><td>MMM</td><td>The abbreviated localized month name (e.g&#x2e; 'Jan' to 'Dec').</td></tr>"
     "<tr valign=\"top\"><td>MMMM</td><td>The long localized month name (e.g&#x2e; 'January' to 'December').</td></tr>"
     "<tr valign=\"top\"><td>yy</td><td>The year as two digit number (00 to 99).</td></tr>"
     "<tr valign=\"top\"><td>yyyy</td><td>The year as four digit number.</td></tr>"
     "</table>"
     "<br />"
     "<table cellpadding=\"2\" cellspacing=\"1\" border=\"1\">"
     "<thead><tr valign=\"top\"><th>Expression</th><th>Output</th></tr></thead>"
     "<tr valign=\"top\"><td>h</td><td>The hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display).</td></tr>"
     "<tr valign=\"top\"><td>hh</td><td>The hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display).</td></tr>"
     "<tr valign=\"top\"><td>H</td><td>The hour without a leading zero (0 to 23, even with AM/PM display).</td></tr>"
     "<tr valign=\"top\"><td>HH</td><td>The hour with a leading zero (00 to 23, even with AM/PM display).</td></tr>"
     "<tr valign=\"top\"><td>m</td><td>The minute without a leading zero (0 to 59).</td></tr>"
     "<tr valign=\"top\"><td>mm</td><td>The minute with a leading zero (00 to 59).</td></tr>"
     "<tr valign=\"top\"><td>s</td><td>The second without a leading zero (0 to 59).</td></tr>"
     "<tr valign=\"top\"><td>ss</td><td>The second with a leading zero (00 to 59).</td></tr>"
     "<tr valign=\"top\"><td>z</td><td>The milliseconds without leading zeroes (0 to 999).</td></tr>"
     "<tr valign=\"top\"><td>zz</td><td>The milliseconds with leading zeroes (000 to 999).</td></tr>"
     "<tr valign=\"top\"><td>AP or A</td><td>Interpret as an AM/PM time. AP must be either 'AM' or 'PM'.</td></tr>"
     "<tr valign=\"top\"><td>ap or a</td><td>Interpret as an AM/PM time. ap must be either 'am' or 'pm'.</td></tr>"
     "</table>"
   "</body>"
   "</html>"
   ));
   dialog->setMainWidget(tb);
   connect(dialog, SIGNAL(okClicked()), dialog, SLOT(close()));

   dialog->exec();

   delete dialog;

}

void ProfileDataInfoDialog::load_text() {
  QString filename = KFileDialog::getOpenFileName(KUrl(QDir::homePath()), "*", this, i18n("Load Text Template"));
  if (!filename.isEmpty()) {
    QFile file(filename);
    if (file.open(QFile::ReadOnly)) {
      QTextStream in(&file);
      ui.ktextedit_text->setPlainText(in.readAll());
      file.close();
    }
  }
}

void ProfileDataInfoDialog::save_text() {
  QString filename = KFileDialog::getSaveFileName(KUrl(QDir::homePath()), "*", this, i18n("Save Text Template"));
  if (!filename.isEmpty()) {
    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
      QTextStream out(&file);
      out << ui.ktextedit_text->toPlainText();
      file.close();
    }
  }
}

bool ProfileDataInfoDialog::save() {
  text = ui.ktextedit_text->toPlainText().split("\n");
  suffix = ui.klineedit_suffix->text();
  pattern = ui.klineedit_pattern->text();
  enableButtonApply(FALSE);
  return TRUE;
}
