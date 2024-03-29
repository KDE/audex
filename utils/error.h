/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ERROR_H
#define ERROR_H

#include <QObject>
#include <QString>

class Error
{
public:
    enum ErrorType { ERROR, WARNING };

    explicit Error(const QString &message = "", const QString &details = "", const ErrorType errorType = Error::ERROR, QObject *parent = nullptr)
    {
        Q_UNUSED(parent);
        p_message = message;
        p_details = details;
        p_type = errorType;
    }
    Error(const Error &other)
    {
        p_message = other.p_message;
        p_details = other.p_details;
        p_type = other.p_type;
    }
    Error &operator=(const Error &other)
    {
        p_message = other.p_message;
        p_details = other.p_details;
        p_type = other.p_type;
        return *this;
    }
    ~Error()
    {
    }

    void clear()
    {
        p_type = Error::ERROR;
        p_message.clear();
        p_details.clear();
    }

    bool operator==(const Error &other) const
    {
        return p_message == other.p_message && p_details == other.p_details && p_type == other.p_type;
    }

    bool operator!=(const Error &other) const
    {
        return p_message != other.p_message || p_details != other.p_details || p_type != other.p_type;
    }

    ErrorType errorType() const
    {
        return p_type;
    }

    bool isValid() const
    {
        return (!p_message.isEmpty());
    }

    const QString message() const
    {
        return p_message;
    }
    const QString details() const
    {
        return p_details;
    }

private:
    ErrorType p_type;
    QString p_message;
    QString p_details;
};

typedef QList<Error> ErrorList;

#endif
