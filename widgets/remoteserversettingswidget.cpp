/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "remoteserversettingswidget.h"

namespace Audex
{

remoteServerSettingsWidget::remoteServerSettingsWidget(QWidget *parent)
    : remoteServerSettingsWidgetUI(parent)
{
    Q_UNUSED(parent);

    QObject::connect(kcfg_upload, &QCheckBox::toggled, this, &remoteServerSettingsWidget::toggle_upload);

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

}
