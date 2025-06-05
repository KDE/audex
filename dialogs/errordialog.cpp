/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "errordialog.h"

#include <KMessageBox>

namespace Audex
{

namespace ErrorDialog
{

void show(QWidget *parent, const QString &message, const QString &details, const QString &caption)
{
    if (details.isEmpty()) {
        KMessageBox::error(parent, message, caption);
    } else {
        KMessageBox::detailedError(parent, message, details, caption);
    }
}

}

}
