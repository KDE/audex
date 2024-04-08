/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "message.h"

namespace Audex
{

Message::Message(const QString &text, const MessageType msgType, const int errorCode, const QString &details)
{
    p_text = text;
    p_type = msgType;
    p_error_code = errorCode;
    p_details = details;
}
Message::Message(const Message &other)
{
    p_text = other.p_text;
    p_type = other.p_type;
    p_error_code = other.p_error_code;
    p_details = other.p_details;
}
Message &Message::operator=(const Message &other)
{
    p_text = other.p_text;
    p_type = other.p_type;
    p_error_code = other.p_error_code;
    p_details = other.p_details;
    return *this;
}
Message::~Message()
{
}

Message::operator bool() const {
    return p_type > WARNING;
}

void Message::clear()
{
    p_type = Message::INFO;
    p_error_code = 0;
    p_text.clear();
    p_details.clear();
}

bool Message::operator==(const Message &other) const
{
    return p_text == other.p_text && p_type == other.p_type && p_error_code == other.p_error_code;
}

bool Message::operator!=(const Message &other) const
{
    return !(*this == other);
}

Message::MessageType Message::type() const
{
    return p_type;
}

bool Message::alert() const
{
    return (p_type != Message::INFO);
}

int Message::errorCode() const
{
    return p_error_code;
}

const QString Message::text() const
{
    return p_text;
}
const QString Message::details() const
{
    return p_details;
}

QDebug operator<<(QDebug debug, const Message &msg)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "(MessageType=" << msg.type() << ", MessageText=" << msg.text() << ", Error Code=" << msg.errorCode() << ", Details=" << msg.details()
                    << ")\n";
    return debug;
}

}
