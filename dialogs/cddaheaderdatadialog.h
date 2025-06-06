/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <KComboBox>
#include <KConfig>
#include <KConfigGroup>

#include "datatypes/metadata.h"
#include "datatypes/toc.h"

#include "ui_cddaheaderdatawidgetUI.h"

namespace Audex
{

class CDDAHeaderDataDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CDDAHeaderDataDialog(const Audex::Metadata::Metadata &metadata, const Audex::Toc::Toc &toc, QWidget *parent = nullptr);
    ~CDDAHeaderDataDialog();

    const Audex::Metadata::Metadata &metadata() const;

private:
    Ui::CDDAHeaderDataWidgetUI ui;
    Audex::Metadata::Metadata p_metadata;
    Audex::Toc::Toc p_toc;
    QPointer<QPushButton> okButton;
};

}
