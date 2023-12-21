/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "generalsettingswidget.h"

#include <KLocalizedString>
#include <QStandardPaths>

generalSettingsWidget::generalSettingsWidget(QWidget *parent)
    : generalSettingsWidgetUI(parent)
{
    urlreq_basePath->setMode(KFile::Directory | KFile::LocalOnly);
    urlreq_basePath->lineEdit()->setObjectName("kcfg_basePath");
}

generalSettingsWidget::~generalSettingsWidget()
{
}
