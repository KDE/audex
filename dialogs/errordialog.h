/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>

class QWidget;

namespace Audex
{

namespace ErrorDialog
{
extern void show(QWidget *parent, const QString &message, const QString &details, const QString &caption = {});
}

}
