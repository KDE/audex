/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatalogfiledialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

ProfileDataLogFileDialog::ProfileDataLogFileDialog(ProfileModel *profile_model, const int profile_row, const bool new_profile_mode, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->profile_model = profile_model;

    if (!this->profile_model) {
        qWarning() << "ProfileDataLogFileDialog() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    this->profile_row = profile_row;
    this->new_profile_mode = new_profile_mode;

    setWindowTitle(i18n("Log Files Settings"));

    // profile data logfile data
    QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_NAME_INDEX)).toString();

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataLogFileDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataLogFileDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &ProfileDataLogFileDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    ui.schemeedit_scheme->setScheme(scheme);
    connect(ui.schemeedit_scheme, &SchemeEdit::edited, this, [this]() {
        trigger_changed();
    });

    if (applyButton)
        applyButton->setEnabled(false);
}

void ProfileDataLogFileDialog::slotAccepted()
{
    if (save())
        accept();
    else
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataLogFileDialog::slotApplied()
{
    if (!save())
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataLogFileDialog::trigger_changed()
{
    if (!profile_model) {
        qWarning() << "ProfileDataLogFileDialog::trigger_changed() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    if (applyButton) {
        const QString scheme = profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_NAME_INDEX)).toString();
        applyButton->setEnabled(ui.schemeedit_scheme->scheme() != scheme);
    }
}

bool ProfileDataLogFileDialog::save()
{
    if (!profile_model) {
        qWarning() << "ProfileDataLogFileDialog::save() called with null model pointers";
        Q_ASSERT(profile_model);
        return false;
    }

    const QString scheme = ui.schemeedit_scheme->scheme();

    error.clear();
    bool success = true;

    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_NAME_INDEX), scheme);

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
