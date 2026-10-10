/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "generalsettingswidget.h"

#include <KFile>
#include <KLineEdit>

#include <QCheckBox>

generalSettingsWidget::generalSettingsWidget(QWidget *parent)
    : generalSettingsWidgetUI(parent)
{
    urlreq_basePath->setMode(KFile::Directory | KFile::LocalOnly);
    urlreq_basePath->lineEdit()->setObjectName("kcfg_basePath");

    // sub-options follow their master checkbox; KConfigXT keeps their values
    const auto follows = [](QCheckBox *master, QWidget *slave) {
        slave->setEnabled(master->isChecked());
        connect(master, &QCheckBox::toggled, slave, &QWidget::setEnabled);
    };
    follows(kcfg_cddbLookupAuto, kcfg_coverLookupAuto); // only used by the automatic lookup
    follows(kcfg_detectGaps, kcfg_readIsrcMcn); // part of the same Q sub-channel scan
    follows(kcfg_detectGaps, label_gapHandling); // the pre-gaps are known from that scan only
    follows(kcfg_detectGaps, kcfg_gapHandling);
    follows(kcfg_hdcdDetect, label_hdcdTag); // the rip checks the tracks only with the detection on
    follows(kcfg_hdcdDetect, kcfg_hdcdTag);
    follows(kcfg_cdgDetect, kcfg_cdgRead); // only discs the detection found with graphics are read
    follows(kcfg_embedCoverScale, kcfg_embedCoverMaxSize);
}

generalSettingsWidget::~generalSettingsWidget()
{
}
