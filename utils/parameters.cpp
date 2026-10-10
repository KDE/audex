/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "parameters.h"

#include "utils/scheme.h"

#include <KLocalizedString>

#include <QList>
#include <QVariant>

Parameters::Parameters()
{
}

Parameters::Parameters(const Parameters &other)
{
    p_parameters = other.p_parameters;
    p_error_string = other.p_error_string;
}

Parameters::Parameters(const QString &string, const QChar &sep)
{
    fromString(string, sep);
}

Parameters &Parameters::operator=(const Parameters &other)
{
    p_parameters = other.p_parameters;
    p_error_string = other.p_error_string;
    return *this;
}

Parameters::~Parameters()
{
}

void Parameters::fromString(const QString &string, const QChar &sep)
{
    p_error_string.clear();
    p_parameters.clear();

    Audex::Scheme::Error error;
    for (const auto &[key, value] : Audex::Scheme::parseKeyValues(string, sep, &error))
        p_parameters.insert(key, value);
    if (error)
        p_error_string = i18n("Invalid parameters at position %1.", error.position + 1);
}

const QString Parameters::toString(const QChar &sep) const
{
    QString string;

    for (auto i = p_parameters.cbegin(), end = p_parameters.cend(); i != end; ++i) {
        const QVariant value = i.value();
        if (i != p_parameters.cbegin())
            string.append(sep);
        if (value.typeId() == QMetaType::QString || value.typeId() == QMetaType::QDateTime || value.typeId() == QMetaType::QDate
            || value.typeId() == QMetaType::QTime)
            string.append(i.key() + QLatin1Char('=') + Audex::Scheme::quoted(value.toString()));
        else
            string.append(i.key() + QLatin1Char('=') + value.toString());
    }

    return string;
}

bool Parameters::contains(const QString &key) const
{
    return p_parameters.contains(key.toLower());
}

const KeyList Parameters::keys() const
{
    return p_parameters.keys();
}
