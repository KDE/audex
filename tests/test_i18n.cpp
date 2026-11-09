/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include <QCoreApplication>

using namespace Audex::Tests;
using namespace Qt::StringLiterals;

namespace
{

struct Expected {
    const char *language;
    const char *timeout;
    const char *compressionLevel;
    const char *canceledShort;
    const char *extractingOne;
    const char *extractingThree;
};

const Expected expectations[] = {
    {"de",
     "Zeitüberschreitung nach 12 s",
     "Kompressionsstufe",
     "Kurz abgebrochen",
     "1 Titel wird nach /music ausgelesen",
     "3 Titel werden nach /music ausgelesen"},
    {"fr",
     "Délai dépassé après 12 s",
     "Niveau de compression",
     "Annulé (bref)",
     "Extraction de 1 piste vers /music",
     "Extraction de 3 pistes vers /music"},
    {"en", "timeout after 12 s", "Compression level", "canceled", "Extracting 1 track to /music", "Extracting 3 tracks to /music"},
};

}

// ctest runs this once per language (see CMakeLists.txt)
AUDEX_TEST("i18n: engine and plugin texts follow LANGUAGE")
{
    const QString language = qEnvironmentVariable("LANGUAGE");
    const Expected *e = nullptr;
    for (const Expected &candidate : expectations)
        if (language == QLatin1String(candidate.language))
            e = &candidate;
    AUDEX_CHECK_MSG(t, e != nullptr, u"no expectations for LANGUAGE=%1"_s.arg(language));
    if (!e)
        return;

    AUDEX_EQUAL(t, QCoreApplication::translate("Audex::Online", "timeout after %1 s").arg(12), QString::fromUtf8(e->timeout));
    AUDEX_EQUAL(t, QCoreApplication::translate("FlacEncoder", "Compression level"), QString::fromUtf8(e->compressionLevel));
    AUDEX_EQUAL(t, QCoreApplication::translate("Audex::Online", "canceled", "short status"), QString::fromUtf8(e->canceledShort));
    AUDEX_EQUAL(t,
                QCoreApplication::translate("Audex::RipJob", "Extracting %n track(s) to %1", nullptr, 1).arg(u"/music"_s),
                QString::fromUtf8(e->extractingOne));
    AUDEX_EQUAL(t,
                QCoreApplication::translate("Audex::RipJob", "Extracting %n track(s) to %1", nullptr, 3).arg(u"/music"_s),
                QString::fromUtf8(e->extractingThree));

    // the context is part of the key
    AUDEX_EQUAL(t, QCoreApplication::translate("UnrelatedContext", "Compression level"), u"Compression level"_s);
}
