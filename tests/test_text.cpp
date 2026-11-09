/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "core/text.h"

#include <QCoreApplication>
#include <QTranslator>

using namespace Audex;
using namespace Audex::Tests;
using namespace Qt::StringLiterals;

namespace
{

// "translates" the texts of the context TextTest by putting them in brackets
class Brackets : public QTranslator
{
public:
    bool isEmpty() const override
    {
        return false;
    }
    QString translate(const char *context, const char *sourceText, const char *, int) const override
    {
        return qstrcmp(context, "TextTest") == 0 ? u"[%1]"_s.arg(QString::fromUtf8(sourceText)) : QString();
    }
};

}

AUDEX_TEST("text: translated for the user interface, English for the log")
{
    Brackets translator;
    QCoreApplication::installTranslator(&translator);

    const Text reason = Text::translate("TextTest", "copies differ");
    const Text line = Text::translate("TextTest", "Track %1: %2").arg(3).arg(reason);
    AUDEX_EQUAL(t, line.toString(), u"[Track 3: [copies differ]]"_s);
    AUDEX_EQUAL(t, line.toEnglish(), u"Track 3: copies differ"_s);

    const Text system = Text::translate("TextTest", "Cannot open %1: %2").arg(u"/dev/sr0"_s).arg(Text::plain(u"Permission denied"_s));
    AUDEX_EQUAL(t, system.toString(), u"[Cannot open /dev/sr0: Permission denied]"_s);
    AUDEX_EQUAL(t, system.toEnglish(), u"Cannot open /dev/sr0: Permission denied"_s);

    const Text sectors = Text::translate("TextTest", "%n sector(s)", nullptr, 2);
    AUDEX_EQUAL(t, sectors.toString(), u"[2 sector(s)]"_s);
    AUDEX_EQUAL(t, sectors.toEnglish(), u"2 sector(s)"_s);

    AUDEX_EQUAL(t, Text::translate("Other", "not translated").toString(), u"not translated"_s);
    AUDEX_CHECK(t, Text().isEmpty());
    AUDEX_CHECK(t, !Text::plain(u"x"_s).isEmpty());
    AUDEX_EQUAL(t, toEnglish({line, Text()}), QStringList({u"Track 3: copies differ"_s, QString()}));
    AUDEX_EQUAL(t, toStrings({line}), QStringList({u"[Track 3: [copies differ]]"_s}));

    QCoreApplication::removeTranslator(&translator);
}
