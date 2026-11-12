/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "models/profilemodel.h"

#include <QDialog>
#include <QPointer>

#include <functional>

class Error;
class QCheckBox;
class QPushButton;
class SchemeEdit;

// Common frame of the small settings dialogs behind the buttons of the
// profile dialog (cover, playlist, cue sheet, log file, command): the
// button box, the Apply button that follows the edits, and saving all
// bound fields to the profile row, all or nothing. A subclass puts its
// form into contentWidget() and binds each field to its model column.
class ProfileDataSubDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataSubDialog(ProfileModel *profileModel, int profileRow, bool newProfileMode, const QString &title, QWidget *parent = nullptr);

protected:
    QWidget *contentWidget() const
    {
        return m_content;
    }
    QVariant storedValue(int column) const; // what the model holds now

    // The value the form shows for a column: compared with the model's to
    // enable Apply, written to the model on OK and Apply in the order bound.
    void bindField(int column, std::function<QVariant()> value);
    // The common widgets: shows the stored value, binds and watches it.
    void bind(QCheckBox *box, int column);
    void bind(SchemeEdit *edit, int column);

protected Q_SLOTS:
    void trigger_changed(); // connect every bound widget's edit signal here

private:
    void slotAccepted();
    void slotApplied();
    bool save(Error *error);

    struct Field {
        int column;
        std::function<QVariant()> value;
    };

    QPointer<ProfileModel> m_model;
    int m_row;
    bool m_newProfile;
    QWidget *m_content = nullptr;
    QPushButton *m_applyButton = nullptr;
    QList<Field> m_fields;
};
