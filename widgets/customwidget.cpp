/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "customwidget.h"

#include <QDebug>

customWidget::customWidget(Parameters *parameters, QWidget *parent)
    : customWidgetUI(parent)
{
    Q_UNUSED(parent);

    this->parameters = parameters;
    if (!parameters) {
        qDebug() << "ParameterString is NULL!";
        return;
    }

    qlineedit_scheme->setText(parameters->value(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, ENCODER_CUSTOM_COMMAND_SCHEME).toString());
    qlineedit_suffix->setText(parameters->value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX).toString());

    connect(qlineedit_suffix, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));
    connect(qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    connect(kpushbutton_scheme, SIGNAL(clicked()), this, SLOT(scheme_wizard()));

    kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    changed = false;
}

customWidget::~customWidget()
{
}

bool customWidget::save()
{
    bool success = true;

    parameters->setValue(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, qlineedit_scheme->text());
    parameters->setValue(ENCODER_CUSTOM_SUFFIX_KEY, qlineedit_suffix->text());

    changed = false;

    return success;
}

void customWidget::scheme_wizard()
{
    CommandWizardDialog *dialog = new CommandWizardDialog(qlineedit_scheme->text(), this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    qlineedit_scheme->setText(dialog->command);

    delete dialog;

    trigger_changed();
}

void customWidget::trigger_changed()
{
    changed = (qlineedit_scheme->text() != parameters->value(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, ENCODER_CUSTOM_COMMAND_SCHEME)
               || qlineedit_suffix->text() != parameters->value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX));

    Q_EMIT triggerChanged();
}
