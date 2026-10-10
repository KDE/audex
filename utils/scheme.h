/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// The placeholder syntax of all schemes of a profile (file names, encoder
// command, command hook), also used for the stored encoder parameters:
//
//   $name  ${name}  ${name key=value key="a value" key='a value'}  $$
//
// A name starts with a letter or "_". "$$" is a dollar sign, a "$" that does
// not start a name stays as it is. A quoted value ends at its quote when the
// quote is followed by a separator or the end; a doubled quote is a quote.
//
// Problems are reported as kind and position; the texts live in the GUI
// (SchemeParser::errorText()).

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <optional>
#include <utility>

namespace Audex::Scheme
{

struct Error {
    enum class Kind {
        None,
        UnclosedBrace,
        IllegalCharacter,
        UnclosedQuote,
        MissingValue, // "key" without "=value"
        ReplaceCharsLength // replace_char_list_from/_to differ in length
    };
    Kind kind = Kind::None;
    qsizetype position = -1; // in the parsed text
    QChar character; // IllegalCharacter

    explicit operator bool() const
    {
        return kind != Kind::None;
    }
};

// keys in lower case, in the order they are written
using KeyValues = QList<std::pair<QString, QVariant>>;

struct Placeholder {
    QString name;
    KeyValues parameters;
    qsizetype position = 0; // of the "$"
    qsizetype length = 0;
};

struct Token {
    QString text; // literal text; for a placeholder the text as written
    std::optional<Placeholder> placeholder;
};

struct Parsed {
    QList<Token> tokens;
    Error error; // tokens end before the error
};

enum Option {
    NoOptions = 0x0,
    BackslashEscape = 0x1 // "\$" is a dollar sign as well (file names of older profiles)
};

Parsed parse(const QString &scheme, int options = NoOptions);

// "key=value<separator>key='value'..." as the profiles store encoder
// parameters. A space as separator stands for any white space.
KeyValues parseKeyValues(const QString &text, QChar separator, Error *error = nullptr);

// A value for a key value list, quoted so that parseKeyValues() reads it back
QString quoted(const QString &value);

// A file name scheme with its values filled in and the parameters applied:
// the value is transformed in the order the parameters are written, pre and
// post are added last. Placeholders without a value are left out.
struct Rendered {
    QString text; // empty on error
    Error error;
    QStringList unknownNames; // as written, e.g. "$foo"
    QList<std::pair<QString, QString>> unknownParameters; // placeholder as written, key
};
Rendered renderFileName(const QString &scheme, const QMap<QString, QString> &values);

// Characters a FAT32 file system does not allow become "_", as do the
// reserved device names (CON, NUL, ...); at most 255 characters
QString fat32Compatible(const QString &name);

}
