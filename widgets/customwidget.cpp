/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "customwidget.h"

#include "dialogs/commandwizarddialog.h"
#include "utils/encoderassistant.h"
#include "utils/encodercommand.h"
#include "utils/parameters.h"

#include <QMenu>

using namespace Qt::StringLiterals;

using Audex::Encoding::EncoderPreset;

customWidget::customWidget(const Parameters &parameters, QWidget *parent)
    : customWidgetUI(parent)
    , p_parameters(parameters)
{
    Q_UNUSED(parent);

    qlineedit_scheme->setText(p_parameters.value(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, ENCODER_CUSTOM_COMMAND_SCHEME).toString());
    qlineedit_suffix->setText(p_parameters.value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX).toString());

    connect(qlineedit_suffix, &QLineEdit::textEdited, this, &customWidget::trigger_changed);
    connect(qlineedit_scheme, &QLineEdit::textEdited, this, &customWidget::trigger_changed);

    connect(kpushbutton_scheme, &QAbstractButton::clicked, this, &customWidget::scheme_wizard);

    kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    const QList<EncoderPreset> presets = Audex::Encoding::encoderPresets();
    auto *presetMenu = new QMenu(qpushbutton_preset);
    for (const EncoderPreset &preset : presets)
        presetMenu->addAction(preset.name, this, [this, preset]() {
            qlineedit_scheme->setText(preset.command);
            qlineedit_suffix->setText(preset.suffix);
            trigger_changed();
        });
    qpushbutton_preset->setMenu(presetMenu);
    qpushbutton_preset->setEnabled(!presets.isEmpty());

    changed = false;
}

customWidget::~customWidget()
{
}

bool customWidget::save()
{
    bool success = true;

    p_parameters.setValue(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, qlineedit_scheme->text());
    p_parameters.setValue(ENCODER_CUSTOM_SUFFIX_KEY, qlineedit_suffix->text().trimmed());

    changed = false;

    return success;
}

void customWidget::scheme_wizard()
{
    CommandWizardDialog dialog(qlineedit_scheme->text(), qlineedit_suffix->text().trimmed(), this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    qlineedit_scheme->setText(dialog.command);
    trigger_changed();
}

void customWidget::trigger_changed()
{
    changed = (qlineedit_scheme->text() != p_parameters.value(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, ENCODER_CUSTOM_COMMAND_SCHEME)
               || qlineedit_suffix->text() != p_parameters.value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX));

    Q_EMIT triggerChanged();
}
