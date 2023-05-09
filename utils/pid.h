/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PID_H
#define PID_H

#include <sys/types.h>
#include <unistd.h>

#include <QObject>

class PID : public QObject
{
    Q_OBJECT

public:
    PID();

    int getPID();
};

#endif
