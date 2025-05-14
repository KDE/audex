/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>
#include <KCrash>

#include "audex-version.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KCrash::initialize();

    KLocalizedString::setApplicationDomain("audex");

    KAboutData aboutData("audex",
                         i18n("Audex"),
                         AUDEX_VERSION_STRING,
                         i18nc("@info", "Audio ripping application"),
                         KAboutLicense::GPL,
                         i18n("Copyright © 2007-2025 Marco Nelles"));
    aboutData.setHomepage("https://apps.kde.org/de/audex/");
    aboutData.setBugAddress("audex@maniatek.de");
    aboutData.addAuthor(i18n("Marco Nelles"), i18n("Current maintainer, main developer"), "marco@maniatek.de");
    aboutData.addCredit(i18n("Craig Drummond"), i18n("GUI improvements, development"), nullptr, "");
    aboutData.addCredit(i18n("Elson"), i18n("development"), nullptr, "");
    aboutData.addCredit(i18n("credativ GmbH"), i18n("Special thanks to credativ GmbH (Germany) for support"), nullptr, "http://www.credativ.de/");
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    aboutData.setOrganizationDomain(QByteArray("kde.org"));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.audex"));

    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("org.kde.audex")));

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    auto *window = new MainWindow();
    window->show();

    return app.exec();
}
