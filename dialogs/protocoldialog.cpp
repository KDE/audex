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

#include "protocoldialog.h"

#include <QFileDialog>
#include <QTextStream>

#include <KLocalizedString>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

ProtocolDialog::ProtocolDialog(const QStringList& protocol, const QString& title, QWidget *parent) : QDialog(parent) {

  Q_UNUSED(parent);

  setWindowTitle(title);


  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Close);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &ProtocolDialog::slotSaveProtocol);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &ProtocolDialog::slotClosed);

  QWidget *widget = new QWidget(this);
  mainLayout->addWidget(widget);
  mainLayout->addWidget(buttonBox);
  ui.setupUi(widget);

  ui.ktextedit->setPlainText(protocol.join("\n"));

  this->protocol = protocol;
  this->title = title;

}

ProtocolDialog::~ProtocolDialog() {

}

void ProtocolDialog::slotClosed() {
  close();
}

void ProtocolDialog::slotSaveProtocol() {
  save();
}

void ProtocolDialog::save() {
  QString fileName = QFileDialog::getSaveFileName(this, i18n("Save %1", title), QDir::homePath(), "*.pro");
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
