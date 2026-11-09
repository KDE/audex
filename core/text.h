/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QStringList>

#include <concepts>

namespace Audex
{

// A text for the user interface that can also go into the rip log, which is
// always English: toString() is the translation, toEnglish() the source text.
// lupdate extracts Text::translate() like QCoreApplication::translate().
// Header-only: the encoder plugins use it without the engine library.
class Text
{
public:
    Text() = default;

    static Text translate(const char *context, const char *sourceText, const char *disambiguation = nullptr, int n = -1)
    {
        Text t;
        t.m_translated = QCoreApplication::translate(context, sourceText, disambiguation, n);
        t.m_english = QString::fromUtf8(sourceText);
        if (n >= 0)
            t.m_english.replace(QStringLiteral("%n"), QString::number(n));
        return t;
    }

    // not translated: names, numbers, messages of the system
    static Text plain(const QString &text)
    {
        Text t;
        t.m_translated = text;
        t.m_english = text;
        return t;
    }

    Text arg(const Text &a) const
    {
        Text t;
        t.m_translated = m_translated.arg(a.m_translated);
        t.m_english = m_english.arg(a.m_english);
        return t;
    }

    Text arg(const QString &a) const
    {
        return arg(plain(a));
    }

    template<std::integral T>
    Text arg(T a) const
    {
        return arg(QString::number(a));
    }

    bool isEmpty() const
    {
        return m_translated.isEmpty() && m_english.isEmpty();
    }

    QString toString() const
    {
        return m_translated;
    }

    QString toEnglish() const
    {
        return m_english;
    }

    bool operator==(const Text &other) const = default;

private:
    QString m_translated;
    QString m_english;
};

inline QStringList toStrings(const QList<Text> &texts)
{
    QStringList l;
    l.reserve(texts.size());
    for (const Text &t : texts)
        l << t.toString();
    return l;
}

inline QStringList toEnglish(const QList<Text> &texts)
{
    QStringList l;
    l.reserve(texts.size());
    for (const Text &t : texts)
        l << t.toEnglish();
    return l;
}

}
