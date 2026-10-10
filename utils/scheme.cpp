/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "scheme.h"

using namespace Qt::StringLiterals;

namespace Audex::Scheme
{

namespace
{

bool isNameChar(QChar c, bool first)
{
    return c.isLetter() || c == u'_' || (!first && c.isDigit());
}

Error makeError(Error::Kind kind, qsizetype position, QChar character = {})
{
    Error e;
    e.kind = kind;
    e.position = position;
    e.character = character;
    return e;
}

// Numbers and true/false unquoted; a number keeps its text only if it reads
// back the same ("01" stays a string)
QVariant typedValue(const QString &value, bool isQuoted)
{
    if (isQuoted)
        return value;
    bool ok = false;
    const int number = value.toInt(&ok);
    if (ok && QString::number(number) == value)
        return number;
    const double real = value.toDouble(&ok);
    if (ok && QString::number(real) == value)
        return real;
    if (value.compare(u"true"_s, Qt::CaseInsensitive) == 0)
        return true;
    if (value.compare(u"false"_s, Qt::CaseInsensitive) == 0)
        return false;
    return value;
}

// Reads key=value pairs from `i` on up to `end` (if not null) or the end of
// the text and returns the position it stopped at.
qsizetype readKeyValues(const QString &s, qsizetype i, QChar separator, QChar end, KeyValues *out, Error *error)
{
    const auto isSeparator = [separator](QChar c) {
        return separator == u' ' ? c.isSpace() : c == separator;
    };
    const auto isEnd = [&s, end](qsizetype at) {
        return !end.isNull() && at < s.size() && s.at(at) == end;
    };

    while (i < s.size() && !isEnd(i)) {
        if (isSeparator(s.at(i)) || s.at(i).isSpace()) {
            ++i;
            continue;
        }

        const qsizetype keyStart = i;
        while (i < s.size() && (s.at(i).isLetterOrNumber() || s.at(i) == u'_'))
            ++i;
        if (i == keyStart) {
            *error = makeError(Error::Kind::IllegalCharacter, i, s.at(i));
            return i;
        }
        if (i >= s.size() || s.at(i) != u'=') {
            *error = makeError(Error::Kind::MissingValue, keyStart);
            return i;
        }
        const QString key = s.sliced(keyStart, i - keyStart).toLower();
        ++i;

        QString value;
        const bool isQuoted = i < s.size() && (s.at(i) == u'"' || s.at(i) == u'\'');
        if (isQuoted) {
            const QChar quote = s.at(i);
            const qsizetype open = i++;
            bool closed = false;
            while (i < s.size()) {
                if (s.at(i) == quote) {
                    if (i + 1 < s.size() && s.at(i + 1) == quote) {
                        value += quote;
                        i += 2;
                        continue;
                    }
                    if (i + 1 >= s.size() || isSeparator(s.at(i + 1)) || isEnd(i + 1)) {
                        ++i;
                        closed = true;
                        break;
                    }
                }
                value += s.at(i++);
            }
            if (!closed) {
                *error = makeError(Error::Kind::UnclosedQuote, open);
                return i;
            }
        } else {
            while (i < s.size() && !isSeparator(s.at(i)) && !isEnd(i)) {
                const QChar c = s.at(i);
                if (c == u'"' || c == u'\'' || c.isSpace()) {
                    *error = makeError(Error::Kind::IllegalCharacter, i, c);
                    return i;
                }
                value += c;
                ++i;
            }
        }
        out->append({key, typedValue(value, isQuoted)});
    }
    return i;
}

QVariant parameter(const KeyValues &parameters, const QString &key)
{
    for (auto it = parameters.crbegin(); it != parameters.crend(); ++it)
        if (it->first == key)
            return it->second;
    return {};
}

}

Parsed parse(const QString &s, int options)
{
    Parsed result;
    QString text;
    const auto flush = [&result, &text]() {
        if (!text.isEmpty())
            result.tokens.append({text, std::nullopt});
        text.clear();
    };

    qsizetype i = 0;
    while (i < s.size()) {
        const QChar c = s.at(i);
        if ((options & BackslashEscape) && c == u'\\' && i + 1 < s.size() && s.at(i + 1) == u'$') {
            text += u'$';
            i += 2;
            continue;
        }
        if (c != u'$') {
            text += c;
            ++i;
            continue;
        }
        if (i + 1 < s.size() && s.at(i + 1) == u'$') {
            text += u'$';
            i += 2;
            continue;
        }

        Placeholder p;
        p.position = i;
        qsizetype j = i + 1;
        if (j < s.size() && s.at(j) == u'{') {
            ++j;
            while (j < s.size() && s.at(j).isSpace())
                ++j;
            const qsizetype start = j;
            while (j < s.size() && isNameChar(s.at(j), j == start))
                ++j;
            if (j == start) {
                flush();
                result.error = j < s.size() ? makeError(Error::Kind::IllegalCharacter, j, s.at(j)) : makeError(Error::Kind::UnclosedBrace, i);
                return result;
            }
            p.name = s.sliced(start, j - start);
            if (j < s.size() && s.at(j) != u'}' && !s.at(j).isSpace()) {
                flush();
                result.error = makeError(Error::Kind::IllegalCharacter, j, s.at(j));
                return result;
            }
            Error error;
            j = readKeyValues(s, j, u' ', u'}', &p.parameters, &error);
            if (!error && j >= s.size())
                error = makeError(Error::Kind::UnclosedBrace, i);
            if (error) {
                flush();
                result.error = error;
                return result;
            }
            ++j; // the closing brace
        } else {
            const qsizetype start = j;
            while (j < s.size() && isNameChar(s.at(j), j == start))
                ++j;
            if (j == start) { // not a name: a plain dollar sign
                text += c;
                ++i;
                continue;
            }
            p.name = s.sliced(start, j - start);
        }

        p.length = j - i;
        flush();
        result.tokens.append({s.sliced(i, p.length), p});
        i = j;
    }
    flush();
    return result;
}

KeyValues parseKeyValues(const QString &text, QChar separator, Error *error)
{
    KeyValues result;
    Error e;
    readKeyValues(text, 0, separator, QChar(), &result, &e);
    if (error)
        *error = e;
    return result;
}

QString quoted(const QString &value)
{
    QString escaped = value;
    return u'\'' + escaped.replace(u'\'', u"''"_s) + u'\'';
}

Rendered renderFileName(const QString &scheme, const QMap<QString, QString> &values)
{
    static const QStringList known{u"lowercase"_s,
                                   u"uppercase"_s,
                                   u"left"_s,
                                   u"underscores"_s,
                                   u"fat32compatible"_s,
                                   u"replace_chars"_s,
                                   u"replace_char_list"_s, // older name of replace_chars
                                   u"replace_char_list_from"_s,
                                   u"replace_char_list_to"_s,
                                   u"length"_s,
                                   u"fillchar"_s,
                                   u"pre"_s,
                                   u"preparam"_s, // older name of pre
                                   u"post"_s,
                                   u"postparam"_s, // older name of post
                                   u"omit_if_empty"_s};

    Rendered result;
    const Parsed parsed = parse(scheme, BackslashEscape);
    if (parsed.error) {
        result.error = parsed.error;
        return result;
    }

    QString text;
    for (const Token &token : parsed.tokens) {
        if (!token.placeholder) {
            text += token.text;
            continue;
        }
        const Placeholder &p = *token.placeholder;
        if (!values.contains(p.name)) {
            if (!result.unknownNames.contains(token.text))
                result.unknownNames.append(token.text);
            continue;
        }

        QString value = values.value(p.name);
        if (parameter(p.parameters, u"omit_if_empty"_s).toBool() && value.isEmpty())
            continue;

        QString pre, post;
        for (const auto &[key, v] : p.parameters) {
            if (!known.contains(key)) {
                result.unknownParameters.append({token.text, key});
            } else if (key == u"lowercase" && v.toBool()) {
                value = value.toLower();
            } else if (key == u"uppercase" && v.toBool()) {
                value = value.toUpper();
            } else if (key == u"left" && v.toInt() > 0) {
                value = value.left(v.toInt());
            } else if (key == u"underscores" && v.toBool()) {
                value.replace(u' ', u'_');
            } else if (key == u"fat32compatible" && v.toBool()) {
                value = fat32Compatible(value);
            } else if ((key == u"replace_chars" || key == u"replace_char_list") && v.toBool()) {
                const QString from = parameter(p.parameters, u"replace_char_list_from"_s).toString();
                const QString to = parameter(p.parameters, u"replace_char_list_to"_s).toString();
                if (from.size() != to.size()) {
                    result.error = makeError(Error::Kind::ReplaceCharsLength, p.position);
                    return result;
                }
                for (qsizetype n = 0; n < from.size(); ++n)
                    value.replace(from.at(n), to.at(n));
            } else if (key == u"length") {
                bool isNumber = false;
                const int number = value.toInt(&isNumber);
                if (isNumber) {
                    const QString fill = parameter(p.parameters, u"fillchar"_s).toString();
                    value = u"%1"_s.arg(number, v.toInt(), 10, fill.isEmpty() ? QChar(u'0') : fill.at(0));
                }
            } else if (key == u"pre" || key == u"preparam") {
                pre = v.toString();
            } else if (key == u"post" || key == u"postparam") {
                post = v.toString();
            }
        }
        text += pre + value + post;
    }
    result.text = text;
    return result;
}

QString fat32Compatible(const QString &name)
{
    static const QString invalid = u"<>:\"/\\|?*"_s;
    QString result;
    result.reserve(name.size());
    for (const QChar c : name)
        result += (invalid.contains(c) || c.unicode() < 0x20) ? QChar(u'_') : c;

    static const QStringList reserved{u"CON"_s,  u"PRN"_s,  u"AUX"_s,  u"NUL"_s,  u"COM1"_s, u"COM2"_s, u"COM3"_s, u"COM4"_s,
                                      u"COM5"_s, u"COM6"_s, u"COM7"_s, u"COM8"_s, u"COM9"_s, u"LPT1"_s, u"LPT2"_s, u"LPT3"_s,
                                      u"LPT4"_s, u"LPT5"_s, u"LPT6"_s, u"LPT7"_s, u"LPT8"_s, u"LPT9"_s};
    if (reserved.contains(result.section(u'.', 0, 0).toUpper()))
        result.prepend(u'_');

    return result.left(255);
}

}
