/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "parameters.h"

Parameters::Parameters()
{
}

Parameters::Parameters(const Parameters &other)
{
    p_parameters = other.p_parameters;
}

Parameters::Parameters(const QString &string, const QChar &sep)
{
    fromString(string, sep);
}

Parameters &Parameters::operator=(const Parameters &other)
{
    p_parameters = other.p_parameters;
    return *this;
}

Parameters::~Parameters()
{
}

void Parameters::fromString(const QString &string, const QChar &sep)
{
    p_error_string.clear();

    if (string.isEmpty())
        return;

    p_parameters.clear();

    QString key, value;
    bool value_is_quoted = false;
    bool is_in_key = false;
    bool is_in_value = false;
    bool is_in_value_quote = false;
    int i = 0;
    while (i < string.length()) {

        QChar c = string.at(i);

        if (is_in_key) {

            if (c.isLetterOrNumber() || c == QChar('_')) {
                key.append(c);
            } else if (c == QChar('=')) {
                is_in_key = false;
                is_in_value = true;
            } else {
                p_error_string = i18n("Illegal character found at index %1: '%2'").arg(i).arg(c);
                return;
            }

        } else if (is_in_value) {

            if (is_in_value_quote) {

                if (c == QChar('\'') || c == QChar('"')) {
                    is_in_value_quote = false;
                    is_in_value = false;
                } else {
                    value.append(c);
                }

            } else {

                if (c == QChar('\'') || c == QChar('"')) {
                    if (value.isEmpty()) {
                        is_in_value_quote = true;
                        value_is_quoted = true;
                    } else {
                        p_error_string = i18n("Illegal character found at index %1: '%2'").arg(i).arg(c);
                        return;
                    }
                } else if (c == sep) {
                    is_in_key = false;
                    is_in_value = false;
                    continue;
                } else if (c.isSpace()) {
                    p_error_string = i18n("Illegal character found at index %1: '%2'").arg(i).arg(c);
                    return;
                } else {
                    value.append(c);
                }

            }

        } else {

            if (c == sep) {
                p_insert_value(key, value, value_is_quoted);
                key.clear();
                value.clear();
                value_is_quoted = false;
            } else if ((c.isLetterOrNumber() || c == QChar('_')) && value.isEmpty()) {
                is_in_key = true;
                key.append(c);
            } else {
                p_error_string = i18n("Illegal character found at index %1: '%2'").arg(i).arg(c);
                return;
            }

        }

        ++i;

    }

    if (is_in_value_quote) {
        p_error_string = i18n("Unclosed quote found");
        return;
    }

    p_insert_value(key, value, value_is_quoted);

}

const QString Parameters::toString(const QChar &sep)
{
    QString string;

    for (auto i = p_parameters.cbegin(), end = p_parameters.cend(); i != end; ++i) {
        QVariant value = i.value();
        if (i != p_parameters.cbegin())
            string.append(sep);
        //qt6: if (value.metaType() == QMetaType::QString || value.metaType() == QMetaType::QDateTime || value.metaType() == QMetaType::QDate || value.metaType() == QMetaType::QTime)
        if (value.type() == QVariant::String || value.type() == QVariant::DateTime || value.type() == QVariant::Date || value.type() == QVariant::Time)
            string.append(i.key() + "='" + value.toString() + "'");
        else
            string.append(i.key() + "=" + value.toString());
    }

    return string;
}

void Parameters::fromBase64(const QByteArray &bytearray)
{
    QByteArray ba = QByteArray::fromBase64(bytearray);
    fromString(QString::fromUtf8(ba.data()));
}

const QByteArray Parameters::toBase64()
{
    QString s = toString();
    return s.toUtf8().toBase64();
}

bool Parameters::contains(const QString &key) const
{
    return p_parameters.contains(key);
}

const KeyList Parameters::keys() const
{
    return p_parameters.keys();
}

bool Parameters::isEmpty() const
{
    return p_parameters.isEmpty();
}

void Parameters::p_insert_value(const QString& key, const QString& value, const bool is_quoted) {

    if (key.isEmpty())
        return;

    // an quoted value is expected to be alway a string
    if (is_quoted) {

        p_parameters.insert(key, QVariant(value));

    } else { // not quoted -> the case is more complicated

        bool ok;
        int number_int = value.toInt(&ok);
        if (ok) {
            p_parameters.insert(key, QVariant(number_int));
        } else {
            double number_double = value.toDouble(&ok);
            if (ok) {
                p_parameters.insert(key, QVariant(number_double));
            } else {

                if (value.toLower() == "false")
                    p_parameters.insert(key, QVariant(false));
                else if (value.toLower() == "true")
                    p_parameters.insert(key, QVariant(true));
                else
                    p_parameters.insert(key, QVariant(value));

            }
        }

    }

}
