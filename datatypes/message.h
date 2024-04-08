/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include <QDebug>
#include <QObject>
#include <QString>

namespace Audex
{

class Message
{
public:
    enum MessageType { INFO = 0, WARNING, ERROR, CRITICAL };

    explicit Message(const QString &text = QString(), const MessageType msgType = Message::INFO, const int errorCode = 0, const QString &details = QString());
    Message(const Message &other);
    Message &operator=(const Message &other);
    ~Message();

    bool operator==(const Message &other) const;
    bool operator!=(const Message &other) const;

    operator bool() const;

    void clear();

    MessageType type() const;
    bool alert() const;

    int errorCode() const;

    const QString text() const;
    const QString details() const;

private:
    MessageType p_type;
    int p_error_code;
    QString p_text;
    QString p_details;
};

typedef QList<Message> MessageList;

QDebug operator<<(QDebug debug, const Message &msg);

}

#endif
