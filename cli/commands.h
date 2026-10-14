/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QStringList>

namespace Audex::Cli
{

// Each takes the whole command line (program, command, options) and
// returns the exit code.
int verify(const QStringList &arguments);
int info(const QStringList &arguments);
int offset(const QStringList &arguments);
int rip(const QStringList &arguments);

}
