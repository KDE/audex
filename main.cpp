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

#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>

#include "config.h"
#include "mainwindow.h"

#include <Kdelibs4ConfigMigrator>

int main (int argc, char *argv[]) {
  Kdelibs4ConfigMigrator migrator(QStringLiteral("audex"));
  migrator.setConfigFiles(QStringList() << QStringLiteral("audexrc"));
  migrator.migrate();

  QApplication app(argc, argv);

  KLocalizedString::setApplicationDomain("audex");

  KAboutData aboutData("audex", i18n("Audex"), AUDEX_VERSION);
  aboutData.setShortDescription(i18n("KDE CDDA Extractor"));
  aboutData.setLicense(KAboutLicense::GPL);
  aboutData.setCopyrightStatement(i18n("Copyright © 2007–2015 by Marco Nelles"));
  aboutData.setHomepage("http://userbase.kde.org/Audex");
  aboutData.setBugAddress("audex@maniatek.com");
  aboutData.addAuthor(i18n("Marco Nelles"), i18n("Current maintainer, main developer"), "marco@maniatek.de");
  aboutData.addCredit(i18n("Craig Drummond"), i18n("GUI improvements, development"), 0, "");
  aboutData.addCredit(i18n("Elson"), i18n("development"), 0, "");
  aboutData.addCredit(i18n("credativ GmbH"), i18n("Special thanks to credativ GmbH (Germany) for support"), 0, "http://www.credativ.com/");
  aboutData.addCredit(i18n("freedb.org"), i18n("Special thanks to freedb.org for providing a free CDDB-like CD database"), 0, "http://freedb.org");
  aboutData.addCredit(i18n("Xiph.Org Foundation"), i18n("Special thanks to Xiph.Org Foundation for providing compact disc ripper"), 0, "http://www.xiph.org/paranoia/index.html");
  aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

  QCommandLineParser parser;
  KAboutData::setApplicationData(aboutData);
  parser.addVersionOption();
  parser.addHelpOption();
  aboutData.setupCommandLine(&parser);
  parser.process(app);
  aboutData.processCommandLine(&parser);

  MainWindow* window = new MainWindow();
  window->show();

  return app.exec();

}
