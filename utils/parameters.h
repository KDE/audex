/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PARAMETERS_HEADER
#define PARAMETERS_HEADER

#include <QList>
#include <QMap>
#include <QMapIterator>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QDebug>

#include <KLocalizedString>

typedef QList<QString> KeyList;

class Parameters
{
public:
    Parameters();
    Parameters(const Parameters &other);
    Parameters(const QString &string, const QChar &sep = ',');
    Parameters &operator=(const Parameters &other);
    ~Parameters();

    void fromString(const QString &string, const QChar &sep = ',');
    const QString toString(const QChar &sep = ',');

    void fromBase64(const QByteArray &bytearray);
    const QByteArray toBase64();

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
    bool isEmpty() const;

    inline bool error() const {
        return !p_error_string.isEmpty();
    }

    inline const QString errorString() const {
        return p_error_string;
    }

private:
    QMap<QString, QVariant> p_parameters;
    QString p_error_string;

    void p_insert_value(const QString& key, const QString& value, const bool is_quoted);

};

#endif
