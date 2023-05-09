/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ERROR_H
#define ERROR_H

class Error
{
public:
    enum ErrorType { ERROR, WARNING };

    explicit Error(const QString &message = "", const QString &details = "", const ErrorType errorType = Error::ERROR, QObject *parent = nullptr)
    {
        Q_UNUSED(parent);
        m = message;
        d = details;
        e = errorType;
    }
    Error(const Error &other)
    {
        m = other.m;
        d = other.d;
        e = other.e;
    }
    Error &operator=(const Error &other)
    {
        m = other.m;
        d = other.d;
        e = other.e;
        return *this;
    }
    ~Error()
    {
    }

    ErrorType errorType() const
    {
        return e;
    }

    bool isValid() const
    {
        return (!m.isEmpty());
    }

    const QString message() const
    {
        return m;
    }
    const QString details() const
    {
        return d;
    }

private:
    ErrorType e;
    QString m;
    QString d;
};

typedef QList<Error> ErrorList;

#endif
