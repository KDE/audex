/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatasinglefiledialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataSingleFileDialog::ProfileDataSingleFileDialog(const QString &pattern, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->pattern = pattern;

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

    connect(ui.kpushbutton_pattern, SIGNAL(clicked()), this, SLOT(pattern_wizard()));
    ui.kpushbutton_pattern->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.qlineedit_pattern->setText(pattern);
    connect(ui.qlineedit_pattern, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

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

void ProfileDataSingleFileDialog::pattern_wizard()
{
    SimplePatternWizardDialog *dialog = new SimplePatternWizardDialog(ui.qlineedit_pattern->text(), "wav", this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    ui.qlineedit_pattern->setText(dialog->pattern);

    delete dialog;

    trigger_changed();
}

void ProfileDataSingleFileDialog::trigger_changed()
{
    if (ui.qlineedit_pattern->text() != pattern) {
        applyButton->setEnabled(true);
        return;
    }
    applyButton->setEnabled(false);
}

bool ProfileDataSingleFileDialog::save()
{
    pattern = ui.qlineedit_pattern->text();
    applyButton->setEnabled(false);
    return true;
}
