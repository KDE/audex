/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatasubdialog.h"

#include "dialogs/errordialog.h"
#include "widgets/schemeedit.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

ProfileDataSubDialog::ProfileDataSubDialog(ProfileModel *profileModel, const int profileRow, const bool newProfileMode, const QString &title, QWidget *parent)
    : QDialog(parent)
    , m_model(profileModel)
    , m_row(profileRow)
    , m_newProfile(newProfileMode)
{
    Q_ASSERT(profileModel);

    setWindowTitle(title);

    // a new profile is saved by the profile dialog, so there is nothing to apply
    QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    if (!m_newProfile)
        buttons |= QDialogButtonBox::Apply;

    auto *buttonBox = new QDialogButtonBox(buttons);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataSubDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    if (!m_newProfile) {
        m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
        m_applyButton->setEnabled(false);
        connect(m_applyButton, &QPushButton::clicked, this, &ProfileDataSubDialog::slotApplied);
    }

    m_content = new QWidget(this);
    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_content);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

QVariant ProfileDataSubDialog::storedValue(const int column) const
{
    return m_model->data(m_model->index(m_row, column));
}

void ProfileDataSubDialog::bindField(const int column, std::function<QVariant()> value)
{
    m_fields.append({column, std::move(value)});
}

void ProfileDataSubDialog::bind(QCheckBox *box, const int column)
{
    box->setChecked(storedValue(column).toBool());
    connect(box, &QAbstractButton::toggled, this, &ProfileDataSubDialog::trigger_changed);
    bindField(column, [box] {
        return box->isChecked();
    });
}

void ProfileDataSubDialog::bind(SchemeEdit *edit, const int column)
{
    edit->setScheme(storedValue(column).toString());
    connect(edit, &SchemeEdit::edited, this, &ProfileDataSubDialog::trigger_changed);
    bindField(column, [edit] {
        return edit->scheme();
    });
}

void ProfileDataSubDialog::trigger_changed()
{
    if (!m_applyButton)
        return;

    bool changed = false;
    for (const Field &field : std::as_const(m_fields)) {
        const QVariant shown = field.value();
        QVariant stored = storedValue(field.column);
        if (stored.metaType() != shown.metaType())
            stored.convert(shown.metaType());
        if (shown != stored) {
            changed = true;
            break;
        }
    }
    m_applyButton->setEnabled(changed);
}

bool ProfileDataSubDialog::save(Error *error)
{
    const Profile before = m_model->profile(m_row);
    for (const Field &field : std::as_const(m_fields)) {
        if (!m_model->setData(m_model->index(m_row, field.column), field.value())) {
            *error = m_model->lastError();
            m_model->restoreProfile(m_row, before);
            return false;
        }
    }

    // a new profile is written by the profile dialog once it is accepted
    if (!m_newProfile)
        m_model->commit();
    if (m_applyButton)
        m_applyButton->setEnabled(false);
    return true;
}

void ProfileDataSubDialog::slotAccepted()
{
    Error error;
    if (save(&error))
        accept();
    else
        ErrorDialog::show(this, error);
}

void ProfileDataSubDialog::slotApplied()
{
    Error error;
    if (!save(&error))
        ErrorDialog::show(this, error);
}
