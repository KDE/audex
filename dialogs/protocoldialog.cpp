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

#include "protocoldialog.h"

ProtocolDialog::ProtocolDialog(const QStringList& protocol, const QString& title, QWidget *parent) : KDialog(parent) {

  Q_UNUSED(parent);

  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);

  setMainWidget(widget);

  setCaption(title);

  setButtons(User1|Close);
  setButtonText(User1, i18n("Save"));
  setButtonIcon(User1, KIcon("document-save-as"));

  ui.ktextedit->setPlainText(protocol.join("\n"));

  this->protocol = protocol;
  this->title = title;
  showButtonSeparator(true);

}

ProtocolDialog::~ProtocolDialog() {

}

void ProtocolDialog::slotButtonClicked(int button) {
  switch(button) {
    case User1:
      save();
      break;
    case Close:
      close();
      break;
    default:
      KDialog::slotButtonClicked(button);
  }
}

void ProtocolDialog::save() {
  QString fileName = KFileDialog::getSaveFileName(KUrl(QDir::homePath()), "*.pro", this, i18n("Save %1", title));
  if (!fileName.isEmpty()) {
    QFile data(fileName);
    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
      QTextStream out(&data);
      out << "AUDEX " << title << "\n";
      out << trUtf8("created on ") << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
      out << "\n";
      for (int i = 0; i < protocol.count(); i++) {
        out << protocol.at(i) << "\n";
      }
    }
  }
}
