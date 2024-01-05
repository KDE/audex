/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatasinglefiledialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataSingleFileDialog::ProfileDataSingleFileDialog(const QString &scheme, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->scheme = scheme;

    setWindowTitle(i18n("Single File Settings"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataSingleFileDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataSingleFileDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &ProfileDataSingleFileDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    connect(ui.kpushbutton_scheme, SIGNAL(clicked()), this, SLOT(scheme_wizard()));
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.qlineedit_scheme->setText(scheme);
    connect(ui.qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

ProfileDataSingleFileDialog::~ProfileDataSingleFileDialog()
{
}

void ProfileDataSingleFileDialog::slotAccepted()
{
    save();
    accept();
}

void ProfileDataSingleFileDialog::slotApplied()
{
    save();
}

void ProfileDataSingleFileDialog::scheme_wizard()
{
    FilenameSchemeWizardDialog *dialog = new FilenameSchemeWizardDialog(ui.qlineedit_scheme->text(), "wav", this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    ui.qlineedit_scheme->setText(dialog->scheme);

    delete dialog;

    trigger_changed();
}

void ProfileDataSingleFileDialog::trigger_changed()
{
    if (ui.qlineedit_scheme->text() != scheme) {
        applyButton->setEnabled(true);
        return;
    }
    applyButton->setEnabled(false);
}

bool ProfileDataSingleFileDialog::save()
{
    scheme = ui.qlineedit_scheme->text();
    applyButton->setEnabled(false);
    return true;
}
