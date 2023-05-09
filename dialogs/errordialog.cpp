// SPDX-FileCopyrightText: 2014  Marco Nelles <marco@maniatek.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "errordialog.h"

#include <KMessageBox>

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
