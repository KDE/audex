// SPDX-FileCopyrightText: 2014  Marco Nelles <marco@maniatek.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <QString>

class QWidget;

namespace ErrorDialog
{
extern void show(QWidget *parent, const QString &message, const QString &details, const QString &caption = {});
}
