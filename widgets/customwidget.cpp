/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "customwidget.h"

#include <QDebug>

namespace Audex
{

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

    QObject::connect(qlineedit_suffix, &QLineEdit::textEdited, this, &customWidget::trigger_changed);
    QObject::connect(qlineedit_scheme, &QLineEdit::textEdited, this, &customWidget::trigger_changed);

    QObject::connect(kpushbutton_scheme, &QPushButton::clicked, this, &customWidget::scheme_wizard);

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

}
