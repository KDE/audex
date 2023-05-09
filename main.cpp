/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>
#include <KCrash>

#include "config.h"
#include "mainwindow.h"

#include <Kdelibs4ConfigMigrator>

int main(int argc, char *argv[])
{
    Kdelibs4ConfigMigrator migrator(QStringLiteral("audex"));
    migrator.setConfigFiles(QStringList() << QStringLiteral("audexrc"));
    migrator.migrate();

    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KCrash::initialize();

    KLocalizedString::setApplicationDomain("audex");

    KAboutData aboutData("audex", i18n("Audex"), AUDEX_VERSION);
    aboutData.setShortDescription(i18n("KDE CDDA Extractor"));
    aboutData.setLicense(KAboutLicense::GPL);
    aboutData.setCopyrightStatement(i18n("Copyright © 2007–2015 by Marco Nelles"));
    aboutData.setHomepage("https://userbase.kde.org/Audex");
    aboutData.setBugAddress("audex@maniatek.com");
    aboutData.addAuthor(i18n("Marco Nelles"), i18n("Current maintainer, main developer"), "marco@maniatek.de");
    aboutData.addCredit(i18n("Craig Drummond"), i18n("GUI improvements, development"), nullptr, "");
    aboutData.addCredit(i18n("Elson"), i18n("development"), nullptr, "");
    aboutData.addCredit(i18n("credativ GmbH"), i18n("Special thanks to credativ GmbH (Germany) for support"), nullptr, "http://www.credativ.com/");
    aboutData.addCredit(i18n("freedb.org"), i18n("Special thanks to freedb.org for providing a free CDDB-like CD database"), nullptr, "http://freedb.org");
    aboutData.addCredit(i18n("Xiph.Org Foundation"), i18n("Special thanks to Xiph.Org Foundation for providing compact disc ripper"), nullptr, "http://www.xiph.org/paranoia/index.html");
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    aboutData.setOrganizationDomain(QByteArray("kde.org"));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.audex"));

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("audex")));

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    auto *window = new MainWindow();
    window->show();

    return app.exec();
}
