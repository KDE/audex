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
    if (string.isEmpty())
        return;

    p_parameters.clear();

    QString option;
    bool is_in_quote = false;
    for (int i = 0; i < string.length(); ++i) {
        if (string[i] == '\'') {
            is_in_quote = !is_in_quote;
        } else if ((string[i] == sep) && (!is_in_quote)) {
            if (option.isEmpty())
                continue;
            int equalPos = option.indexOf('=');
            if (equalPos == -1) {
                p_parameters[option] = "";
            } else {
                QString ropt = option.left(equalPos);
                QString val = option.mid(equalPos + 1);
                p_parameters[ropt] = val.trimmed();
            }
            option.clear();
        } else {
            option += string[i];
        }
    }

    int equalPos = option.indexOf('=');
    if (equalPos == -1) {
        p_parameters[option] = "";
    } else {
        QString ropt = option.left(equalPos);
        QString val = option.mid(equalPos + 1);
        p_parameters[ropt] = val.trimmed();
    }
}

const QString Parameters::toString(const QChar &sep)
{
    QString string;

    QHash<QString, QString>::const_iterator i = p_parameters.constBegin();
    bool first = true;
    while (i != p_parameters.constEnd()) {
        QString s = i.value();
        s.replace('\'', "\\'");
        if (!first) {
            string += sep + i.key() + "='" + s + '\'';
        } else {
            string += i.key() + "='" + s + '\'';
            first = false;
        }
        ++i;
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
