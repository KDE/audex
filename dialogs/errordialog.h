/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>

#include "utils/error.h"

class QWidget;

namespace ErrorDialog
{
// For errors with text from outside Audex (a server, a library, the
// system): that text goes into a box below the message, from where it can
// be copied into a bug report or a search. Without details it is the usual
// KDE message box.
extern void show(QWidget *parent, const QString &message, const QString &details, const QString &caption = {}, Error::ErrorType type = Error::ERROR);

// For errors Audex explains itself, e.g. of a setting: the usual KDE
// message box, with the details as a second paragraph.
extern void show(QWidget *parent, const Error &error, const QString &caption = {});
}
