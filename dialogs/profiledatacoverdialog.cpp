/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatacoverdialog.h"

#include <QDialogButtonBox>
#include <QPushButton>

ProfileDataCoverDialog::ProfileDataCoverDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;
    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    if (!profile_model) {
        qWarning() << "ProfileDataCoverDialog() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    const bool scale = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_SCALE_INDEX)).toBool();
    const QSize size = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_SIZE_INDEX)).toSize();
    const QString format = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX)).toString();
    const QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_NAME_INDEX)).toString();

    setWindowTitle(i18n("Cover Settings"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    if (!new_profile_mode)
        buttons |= QDialogButtonBox::Apply;

    QDialogButtonBox *buttonBox = new QDialogButtonBox(buttons);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    if (!new_profile_mode)
        applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataCoverDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataCoverDialog::reject);
    if (!new_profile_mode)
        connect(applyButton, &QPushButton::clicked, this, &ProfileDataCoverDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    ui.checkBox_scale->setChecked(scale);
    enable_scale(ui.checkBox_scale->isChecked());
    connect(ui.checkBox_scale, &QAbstractButton::toggled, this, &ProfileDataCoverDialog::trigger_changed);
    connect(ui.checkBox_scale, &QAbstractButton::toggled, this, &ProfileDataCoverDialog::enable_scale);

    ui.kintspinbox_x->setValue(size.width());
    connect(ui.kintspinbox_x, &QSpinBox::valueChanged, this, &ProfileDataCoverDialog::trigger_changed);

    ui.kintspinbox_y->setValue(size.height());
    connect(ui.kintspinbox_y, &QSpinBox::valueChanged, this, &ProfileDataCoverDialog::trigger_changed);

    ui.checkBox_png->setChecked(format == "PNG"); // anything else (legacy BMP etc.) means JPEG
    connect(ui.checkBox_png, &QAbstractButton::toggled, this, &ProfileDataCoverDialog::trigger_changed);

    ui.schemeedit_scheme->setScheme(scheme);
    connect(ui.schemeedit_scheme, &SchemeEdit::edited, this, &ProfileDataCoverDialog::trigger_changed);

    if (applyButton)
        applyButton->setEnabled(false);
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

void ProfileDataCoverDialog::trigger_changed()
{
    if (!profile_model) {
        qWarning() << "ProfileDataCoverDialog::trigger_changed() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    if (applyButton) {
        const bool scale = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_SCALE_INDEX)).toBool();
        const QSize size = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_SIZE_INDEX)).toSize();
        const QString format = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX)).toString();
        const QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_NAME_INDEX)).toString();

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
        if ((ui.checkBox_png->isChecked() ? QStringLiteral("PNG") : QStringLiteral("JPEG")) != format) {
            applyButton->setEnabled(true);
            return;
        }
        if (ui.schemeedit_scheme->scheme() != scheme) {
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
    if (!profile_model) {
        qWarning() << "ProfileDataCoverDialog::save() called with null model pointers";
        Q_ASSERT(profile_model);
        return false;
    }

    const bool scale = ui.checkBox_scale->isChecked();
    const QSize size = QSize(ui.kintspinbox_x->value(), ui.kintspinbox_y->value());
    const QString format = ui.checkBox_png->isChecked() ? QStringLiteral("PNG") : QStringLiteral("JPEG");
    const QString scheme = ui.schemeedit_scheme->scheme();

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
