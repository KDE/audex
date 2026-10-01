/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>
#include <KCrash>
#include <KLocalizedString>

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
                         i18n("Copyright © 2007-2026 Marco Nelles"));
    aboutData.setHomepage("https://apps.kde.org/audex/");
    aboutData.setBugAddress("https://bugs.kde.org/enter_bug.cgi?product=audex");
    aboutData.addAuthor(i18n("Marco Nelles"), i18n("Current maintainer, main developer"), "marco@maniatek.de");
    aboutData.addCredit(i18n("Craig Drummond"), i18n("GUI improvements, development"), nullptr, "");
    aboutData.addCredit(i18n("Elson"), i18n("development"), nullptr, "");
    aboutData.addCredit(i18n("MusicBrainz"), i18n("Provides online disc lookup database"), QString(), QStringLiteral("https://musicbrainz.org"));
    aboutData.addCredit(i18n("AccurateRip"), i18n("Provides CD ripping accuracy verification"), QString(), QStringLiteral("http://www.accuraterip.com"));
    aboutData.addCredit(i18n("CUETools database"),
                        i18n("Provides rip verification and recovery data"),
                        QString(),
                        QStringLiteral("http://cue.tools/wiki/CUETools_Database"));
    aboutData.addCredit(i18n("Gregory S. Chudov"),
                        i18n("CUETools; the CTDB verification and repair follow its implementation"),
                        QString(),
                        QStringLiteral("https://github.com/gchudov/cuetools.net"));
    aboutData.addCredit(i18n("Thomas Schmitt"),
                        i18n("libburn; the CD-R/RW manufacturer codes follow its table"),
                        QString(),
                        QStringLiteral("https://libburnia-project.org"));
    aboutData.addCredit(i18n("Chris Moeller"),
                        i18n("FFmpeg's HDCD filter; the HDCD detection is ported from it"),
                        QString(),
                        QStringLiteral("https://ffmpeg.org"));
    aboutData.addCredit(i18n("credativ GmbH"), i18n("Special thanks to credativ GmbH (Germany)"), nullptr, "http://www.credativ.de");
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
    if (!window->isValid()) {
        // error has already been shown by the constructor
        delete window;
        return 1;
    }
    window->show();

    return app.exec();
}
