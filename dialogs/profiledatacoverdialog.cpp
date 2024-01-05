/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatacoverdialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataCoverDialog::ProfileDataCoverDialog(const bool scale, const QSize &size, const QString &format, const QString &scheme, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->scale = scale;
    this->size = size;
    this->format = format;
    this->scheme = scheme;

    setWindowTitle(i18n("Cover Settings"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataCoverDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataCoverDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &ProfileDataCoverDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    connect(ui.kpushbutton_scheme, SIGNAL(clicked()), this, SLOT(scheme_wizard()));
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.checkBox_scale->setChecked(scale);
    enable_scale(ui.checkBox_scale->isChecked());
    connect(ui.checkBox_scale, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));
    connect(ui.checkBox_scale, SIGNAL(toggled(bool)), this, SLOT(enable_scale(bool)));

    ui.kintspinbox_x->setValue(size.width());
    connect(ui.kintspinbox_x, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

    ui.kintspinbox_y->setValue(size.height());
    connect(ui.kintspinbox_y, SIGNAL(valueChanged(int)), this, SLOT(trigger_changed()));

    ui.kcombobox_format->addItem(i18n("JPEG (Joint Photographic Experts Group)"), "JPEG");
    ui.kcombobox_format->addItem(i18n("PNG (Portable Network Graphics)"), "PNG");
    ui.kcombobox_format->addItem(i18n("BMP (Windows Bitmap)"), "BMP");
    {
        int i = ui.kcombobox_format->findData(format);
        ui.kcombobox_format->setCurrentIndex(i);
    }
    connect(ui.kcombobox_format, SIGNAL(currentIndexChanged(int)), this, SLOT(trigger_changed()));

    ui.qlineedit_scheme->setText(scheme);
    connect(ui.qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    applyButton->setEnabled(false);
}

ProfileDataCoverDialog::~ProfileDataCoverDialog()
{
}

void ProfileDataCoverDialog::slotAccepted()
{
    save();
    accept();
}

void ProfileDataCoverDialog::slotApplied()
{
    save();
}

void ProfileDataCoverDialog::scheme_wizard()
{
    QString suffix = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString().toLower();

    FilenameSchemeWizardDialog *dialog = new FilenameSchemeWizardDialog(ui.qlineedit_scheme->text(), suffix, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    ui.qlineedit_scheme->setText(dialog->scheme);

    delete dialog;

    trigger_changed();
}

void ProfileDataCoverDialog::trigger_changed()
{
    if (ui.checkBox_scale->isChecked() != scale) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.kintspinbox_x->value() != size.width()) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.kintspinbox_y->value() != size.height()) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString() != format) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.qlineedit_scheme->text() != scheme) {
        applyButton->setEnabled(true);
        return;
    }
    applyButton->setEnabled(false);
}

void ProfileDataCoverDialog::enable_scale(bool enabled)
{
    ui.label_x->setEnabled(enabled);
    ui.kintspinbox_x->setEnabled(enabled);
    ui.kintspinbox_y->setEnabled(enabled);
}

bool ProfileDataCoverDialog::save()
{
    scale = ui.checkBox_scale->isChecked();
    size = QSize(ui.kintspinbox_x->value(), ui.kintspinbox_y->value());
    format = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString();
    scheme = ui.qlineedit_scheme->text();
    applyButton->setEnabled(false);
    return true;
}
