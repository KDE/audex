/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "metadatacandidatedialog.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QVBoxLayout>

#include <KLocalizedString>

using namespace Qt::StringLiterals;

MetadataCandidateDialog::MetadataCandidateDialog(const Audex::MetadataCandidates &candidates, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Select Album Information"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    list = new QListWidget(this);
    for (const Audex::MetadataCandidate &candidate : candidates) {
        auto *item = new QListWidgetItem(u"%1: %2"_s.arg(candidate.providerName, candidate.description()), list);
        if (candidate.score > 0)
            item->setToolTip(i18n("Match quality: %1%", candidate.score));
    }
    list->setCurrentRow(0);
    connect(list, &QListWidget::doubleClicked, this, &QDialog::accept);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(list);
    mainLayout->addWidget(buttonBox);

    resize(560, 320);
}

int MetadataCandidateDialog::selectedCandidate() const
{
    return list->currentRow();
}
