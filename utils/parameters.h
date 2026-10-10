/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QList>
#include <QMap>
#include <QVariant>

class QString;
class QChar;

typedef QList<QString> KeyList;

// Key value list as the profiles store it, read with the syntax of utils/scheme.h
class Parameters
{
public:
    Parameters();
    Parameters(const Parameters &other);
    Parameters(const QString &string, const QChar &sep = ',');
    Parameters &operator=(const Parameters &other);
    ~Parameters();

    void fromString(const QString &string, const QChar &sep = ',');
    const QString toString(const QChar &sep = ',') const;

    inline void setValue(const QString &key, const QVariant &value)
    {
        p_parameters.insert(key.toLower(), value);
    }

    inline const QVariant value(const QString &key, const QVariant &def = QVariant()) const
    {
        return p_parameters.value(key.toLower(), def);
    }

    bool contains(const QString &key) const;
    const KeyList keys() const;

    inline bool error() const
    {
        return !p_error_string.isEmpty();
    }

    inline const QString errorString() const
    {
        return p_error_string;
    }

private:
    QMap<QString, QVariant> p_parameters;
    QString p_error_string;
};
