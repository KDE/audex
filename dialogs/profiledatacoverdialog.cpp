/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatacoverdialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Audex
{

ProfileDataCoverDialog::ProfileDataCoverDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;
    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    applyButton = nullptr;

    bool scale = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_SCALE_INDEX)).toBool();
    QSize size = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_SIZE_INDEX)).toSize();
    QString format = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX)).toString();
    QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_NAME_INDEX)).toString();

    setWindowTitle(i18n("Cover Settings"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    if (!new_profile_mode)
        buttons |= QDialogButtonBox::Apply;

    auto *buttonBox = new QDialogButtonBox(buttons);
    auto *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    if (!new_profile_mode)
        applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataCoverDialog::slotAccepted);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataCoverDialog::reject);
    if (!new_profile_mode)
        QObject::connect(applyButton, &QPushButton::clicked, this, &ProfileDataCoverDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    QObject::connect(ui.kpushbutton_scheme, &QPushButton::clicked, this, &ProfileDataCoverDialog::scheme_wizard);
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.checkBox_scale->setChecked(scale);
    enable_scale(ui.checkBox_scale->isChecked());
    QObject::connect(ui.checkBox_scale, &QCheckBox::toggled, this, &ProfileDataCoverDialog::trigger_changed);
    QObject::connect(ui.checkBox_scale, &QCheckBox::toggled, this, &ProfileDataCoverDialog::enable_scale);

    ui.kintspinbox_x->setValue(size.width());
    QObject::connect(ui.kintspinbox_x, &QSpinBox::valueChanged, this, &ProfileDataCoverDialog::trigger_changed);

    ui.kintspinbox_y->setValue(size.height());
    QObject::connect(ui.kintspinbox_y, &QSpinBox::valueChanged, this, &ProfileDataCoverDialog::trigger_changed);

    ui.kcombobox_format->addItem(i18n("JPEG (Joint Photographic Experts Group)"), "JPEG");
    ui.kcombobox_format->addItem(i18n("PNG (Portable Network Graphics)"), "PNG");
    ui.kcombobox_format->addItem(i18n("BMP (Windows Bitmap)"), "BMP");
    {
        int i = ui.kcombobox_format->findData(format);
        ui.kcombobox_format->setCurrentIndex(i);
    }
    QObject::connect(ui.kcombobox_format, &KComboBox::currentIndexChanged, this, &ProfileDataCoverDialog::trigger_changed);

    ui.qlineedit_scheme->setText(scheme);
    QObject::connect(ui.qlineedit_scheme, &QLineEdit::textEdited, this, &ProfileDataCoverDialog::trigger_changed);

    if (applyButton)
        applyButton->setEnabled(false);
}

ProfileDataCoverDialog::~ProfileDataCoverDialog()
{
}

void ProfileDataCoverDialog::slotAccepted()
{
    if (save())
        accept();
    else
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataCoverDialog::slotApplied()
{
    if (!save())
        ErrorDialog::show(this, error.message(), error.details());
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
    if (applyButton) {
        bool scale = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_SCALE_INDEX)).toBool();
        QSize size = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_SIZE_INDEX)).toSize();
        QString format = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX)).toString();
        QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_NAME_INDEX)).toString();

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
}

void ProfileDataCoverDialog::enable_scale(bool enabled)
{
    ui.label_x->setEnabled(enabled);
    ui.kintspinbox_x->setEnabled(enabled);
    ui.kintspinbox_y->setEnabled(enabled);
}

bool ProfileDataCoverDialog::save()
{
    bool scale = ui.checkBox_scale->isChecked();
    QSize size = QSize(ui.kintspinbox_x->value(), ui.kintspinbox_y->value());
    QString format = ui.kcombobox_format->itemData(ui.kcombobox_format->currentIndex()).toString();
    QString scheme = ui.qlineedit_scheme->text();

    error.clear();
    bool success = true;

    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_SCALE_INDEX), scale);
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_SIZE_INDEX), size);
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX), format);
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_NAME_INDEX), scheme);

    if (!success)
        error = profile_model->lastError();

    if (success) {
        profile_model->commit();
        if (applyButton)
            applyButton->setEnabled(false);
        return true;
    }

    return false;
}

}
