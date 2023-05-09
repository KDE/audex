/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "remoteserversettingswidget.h"

remoteServerSettingsWidget::remoteServerSettingsWidget(QWidget *parent)
    : remoteServerSettingsWidgetUI(parent)
{
    Q_UNUSED(parent);

    connect(kcfg_upload, SIGNAL(toggled(bool)), this, SLOT(toggle_upload(bool)));
    toggle_upload(kcfg_upload->isChecked());
    kcfg_url->setMode(KFile::Directory | KFile::ExistingOnly | KFile::File); // Don't want files, but KDirSelect doesn't seem to like remote urls :-(
}

remoteServerSettingsWidget::~remoteServerSettingsWidget()
{
}

void remoteServerSettingsWidget::toggle_upload(bool enabled)
{
    kcfg_url->setEnabled(enabled);
    label->setEnabled(enabled);
}
