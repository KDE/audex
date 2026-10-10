/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "utils/scheme.h"

using namespace Audex::Scheme;
using namespace Qt::StringLiterals;

namespace
{

const QMap<QString, QString> Values{{u"artist"_s, u"AC DC"_s},
                                    {u"ttitle"_s, u"Hells Bells"_s},
                                    {u"trackno"_s, u"02"_s},
                                    {u"cdno"_s, QString()},
                                    {u"suffix"_s, u"flac"_s}};

QString render(const QString &scheme)
{
    const Rendered r = renderFileName(scheme, Values);
    return r.error ? u"error %1 at %2"_s.arg(int(r.error.kind)).arg(r.error.position) : r.text;
}

}

AUDEX_TEST("scheme: placeholders, escapes and plain dollar signs")
{
    AUDEX_EQUAL(t, render(u"$artist/$trackno - ${ttitle}.$suffix"_s), u"AC DC/02 - Hells Bells.flac"_s);
    AUDEX_EQUAL(t, render(u"$$artist \\$artist"_s), u"$artist $artist"_s);
    AUDEX_EQUAL(t, render(u"Price $5 $ x$"_s), u"Price $5 $ x$"_s);
    AUDEX_EQUAL(t, render(u"$artistX$trackno"_s), u"02"_s);
    AUDEX_EQUAL(t, renderFileName(u"$artistX"_s, Values).unknownNames, QStringList{u"$artistX"_s});
}

AUDEX_TEST("scheme: parameters apply in written order, pre and post last")
{
    AUDEX_EQUAL(t, render(u"${ttitle uppercase=true pre=\"by \"}"_s), u"by HELLS BELLS"_s);
    AUDEX_EQUAL(t, render(u"${artist pre=\"CD \" underscores=true}"_s), u"CD AC_DC"_s);
    AUDEX_EQUAL(t, render(u"${ttitle left=5 uppercase=true}"_s), u"HELLS"_s);
    AUDEX_EQUAL(t, render(u"${ttitle Uppercase=TRUE}"_s), u"HELLS BELLS"_s);
}

AUDEX_TEST("scheme: length works on number strings")
{
    AUDEX_EQUAL(t, render(u"${trackno length=3}"_s), u"002"_s);
    AUDEX_EQUAL(t, render(u"${trackno length=4 fillchar=_}"_s), u"___2"_s);
    AUDEX_EQUAL(t, render(u"${ttitle length=3}"_s), u"Hells Bells"_s);
}

AUDEX_TEST("scheme: quotes and braces in values")
{
    AUDEX_EQUAL(t, render(u"${trackno pre=\"{\" post=\"}/\"}"_s), u"{02}/"_s);
    AUDEX_EQUAL(t, render(u"${ttitle pre=\"Rock'n'Roll \"}"_s), u"Rock'n'Roll Hells Bells"_s);
    AUDEX_EQUAL(t, render(u"${ttitle pre='it''s '}"_s), u"it's Hells Bells"_s);
    AUDEX_EQUAL(t, render(u"${cdno omit_if_empty=true pre=\"CD \" post=\"/\"}x"_s), u"x"_s);
}

AUDEX_TEST("scheme: errors")
{
    AUDEX_EQUAL(t, qint64(renderFileName(u"${ttitle"_s, Values).error.kind), qint64(Error::Kind::UnclosedBrace));
    AUDEX_EQUAL(t, qint64(renderFileName(u"${tti-tle}"_s, Values).error.kind), qint64(Error::Kind::IllegalCharacter));
    AUDEX_EQUAL(t, qint64(renderFileName(u"${ttitle pre=\"x}"_s, Values).error.kind), qint64(Error::Kind::UnclosedQuote));
    AUDEX_EQUAL(t, qint64(renderFileName(u"${ttitle lowercase}"_s, Values).error.kind), qint64(Error::Kind::MissingValue));
    AUDEX_EQUAL(t,
                qint64(renderFileName(u"${ttitle replace_chars=true replace_char_list_from=ab}"_s, Values).error.kind),
                qint64(Error::Kind::ReplaceCharsLength));
    const Rendered r = renderFileName(u"${ttitle lowercse=true}"_s, Values);
    AUDEX_EQUAL(t, qint64(r.unknownParameters.size()), qint64(1));
}

AUDEX_TEST("scheme: stored key value lists")
{
    const QString command = u"ffmpeg -metadata comment=\"Ripped by Audex\" -o 'x' $o"_s;
    const QString stored = u"command_scheme="_s + quoted(command) + u",suffix=m4a,embed_cover=true"_s;
    Error error;
    const KeyValues kv = parseKeyValues(stored, u',', &error);
    AUDEX_CHECK(t, !error);
    AUDEX_EQUAL(t, qint64(kv.size()), qint64(3));
    AUDEX_EQUAL(t, kv.value(0).second.toString(), command);
    AUDEX_CHECK(t, kv.value(2).second.typeId() == QMetaType::Bool);

    // written by older versions without doubling the quotes
    AUDEX_EQUAL(t, parseKeyValues(u"command_scheme='oggenc -o \"$o\" \"$i\"',suffix='ogg'"_s, u',').value(0).second.toString(),
                u"oggenc -o \"$o\" \"$i\""_s);
    AUDEX_EQUAL(t, parseKeyValues(u"a='it's',b=1"_s, u',').value(0).second.toString(), u"it's"_s);
    AUDEX_EQUAL(t, parseKeyValues(u"suffix=001"_s, u',').value(0).second.toString(), u"001"_s);
}
