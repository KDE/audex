/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "utils/enginetranslator.h"

#include <KLocalizedString>

#include <QCoreApplication>

using namespace Audex::Tests;

AUDEX_TEST("i18n: GUI, engine and plugins use the same language")
{
    KLocalizedString::addDomainLocaleDir("audex", QStringLiteral(AUDEX_TEST_LOCALE_DIR));
    KLocalizedString::setApplicationDomain("audex");

    EngineTranslator translator;
    AUDEX_CHECK(t, QCoreApplication::installTranslator(&translator));

    KLocalizedString::setLanguages({QStringLiteral("de")});
    AUDEX_EQUAL(t, i18n("Rip And Encode"), QStringLiteral("Auslesen und Kodieren"));
    AUDEX_EQUAL(t, i18nc("@info", "Audio ripping application"), QStringLiteral("CD-Audio-Extraktionsprogramm"));
    AUDEX_EQUAL(t, QCoreApplication::translate("Audex::Online", "timeout after %1 s").arg(12), QStringLiteral("Zeitüberschreitung nach 12 s"));
    AUDEX_EQUAL(t, QCoreApplication::translate("FlacEncoder", "Compression level"), QStringLiteral("Kompressionsstufe"));
    AUDEX_EQUAL(t, QCoreApplication::translate("Audex::Online", "canceled", "short status"), QStringLiteral("Kurz abgebrochen"));

    KLocalizedString::setLanguages({QStringLiteral("fr")});
    AUDEX_EQUAL(t, i18n("Rip And Encode"), QStringLiteral("Extraire et encoder"));
    AUDEX_EQUAL(t, i18nc("@info", "Audio ripping application"), QStringLiteral("Application d'extraction audio"));
    AUDEX_EQUAL(t, QCoreApplication::translate("Audex::Online", "timeout after %1 s").arg(12), QStringLiteral("Délai dépassé après 12 s"));
    AUDEX_EQUAL(t, QCoreApplication::translate("FlacEncoder", "Compression level"), QStringLiteral("Niveau de compression"));
    AUDEX_EQUAL(t, QCoreApplication::translate("Audex::Online", "canceled", "short status"), QStringLiteral("Annulé (bref)"));

    KLocalizedString::setLanguages({QStringLiteral("en_US")});
    AUDEX_EQUAL(t, i18n("Rip And Encode"), QStringLiteral("Rip And Encode"));
    AUDEX_EQUAL(t, QCoreApplication::translate("Audex::Online", "timeout after %1 s").arg(12), QStringLiteral("timeout after 12 s"));
    AUDEX_EQUAL(t, QCoreApplication::translate("FlacEncoder", "Compression level"), QStringLiteral("Compression level"));

    KLocalizedString::setLanguages({QStringLiteral("de")});
    AUDEX_EQUAL(t, i18n("Rip And Encode"), QStringLiteral("Auslesen und Kodieren"));
    AUDEX_EQUAL(t, QCoreApplication::translate("Audex::Online", "timeout after %1 s").arg(12), QStringLiteral("Zeitüberschreitung nach 12 s"));

    AUDEX_EQUAL(t, QCoreApplication::translate("UnrelatedContext", "Compression level"), QStringLiteral("Compression level"));
    AUDEX_CHECK(t, translator.translate("UnrelatedContext", "Compression level").isNull());

    QCoreApplication::removeTranslator(&translator);
    KLocalizedString::clearLanguages();
}
