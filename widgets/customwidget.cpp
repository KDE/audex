/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "customwidget.h"

#include "encoding/tagwriter.h"
#include "utils/encoderassistant.h"
#include "utils/encodercommand.h"
#include "utils/parameters.h"

#include <KLocalizedString>

#include <QAbstractButton>
#include <QMenu>

using namespace Qt::StringLiterals;

using Audex::Encoding::EncoderPreset;

customWidget::customWidget(const Parameters &parameters, QWidget *parent)
    : customWidgetUI(parent)
    , p_parameters(parameters)
{
    Q_UNUSED(parent);

    schemeedit_command->setScheme(p_parameters.value(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, ENCODER_CUSTOM_COMMAND_SCHEME).toString());
    qlineedit_suffix->setText(p_parameters.value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX).toString());
    checkBox_tags->setChecked(p_parameters.value(ENCODER_CUSTOM_TAGS_KEY, ENCODER_CUSTOM_TAGS).toBool());
    checkBox_embedcover->setChecked(p_parameters.value(ENCODER_CUSTOM_EMBED_COVER_KEY, ENCODER_CUSTOM_EMBED_COVER).toBool());
    checkBox_embedcover->setEnabled(checkBox_tags->isChecked());
    // depends on the TagLib Audex is built with
    if (Audex::Encoding::TagWriter::coverInAllFormats())
        checkBox_embedcover->setToolTip(
            i18n("<p>Puts the cover image into the music file, so that players show it without a separate image file.</p>"
                 "<p>This works for all formats listed under \"Write tags to music file\" that can hold a picture.</p>"));
    else
        checkBox_embedcover->setToolTip(
            i18n("<p>Puts the cover image into the music file, so that players show it without a separate image file.</p>"
                 "<p>This works for MP3, FLAC, Ogg Vorbis, Opus and M4A. Other formats (e.g. WavPack or APE) get the tags, but no cover: "
                 "this Audex is built with an older version of TagLib, the library it uses for tags. "
                 "With TagLib 2 or newer, the cover goes into these files as well.</p>"));

    connect(qlineedit_suffix, &QLineEdit::textEdited, this, &customWidget::trigger_changed);
    connect(checkBox_tags, &QAbstractButton::toggled, checkBox_embedcover, &QWidget::setEnabled);
    connect(checkBox_tags, &QAbstractButton::toggled, this, &customWidget::trigger_changed);
    connect(checkBox_embedcover, &QAbstractButton::toggled, this, &customWidget::trigger_changed);
    connect(schemeedit_command, &SchemeEdit::edited, this, &customWidget::trigger_changed);

    const QList<EncoderPreset> presets = Audex::Encoding::encoderPresets();
    auto *presetMenu = new QMenu(qpushbutton_preset);
    for (const EncoderPreset &preset : presets)
        presetMenu->addAction(preset.name, this, [this, preset]() {
            schemeedit_command->setScheme(preset.command);
            qlineedit_suffix->setText(preset.suffix);
            trigger_changed();
        });
    qpushbutton_preset->setMenu(presetMenu);
    qpushbutton_preset->setEnabled(!presets.isEmpty());
    if (presets.isEmpty())
        qpushbutton_preset->setToolTip(i18n("No encoder presets found: encoderpresets.json is not installed, or not where Audex looks for its data."));

    changed = false;
}

customWidget::~customWidget()
{
}

bool customWidget::save()
{
    bool success = true;

    p_parameters.setValue(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, schemeedit_command->scheme());
    p_parameters.setValue(ENCODER_CUSTOM_SUFFIX_KEY, qlineedit_suffix->text().trimmed());
    p_parameters.setValue(ENCODER_CUSTOM_TAGS_KEY, checkBox_tags->isChecked());
    p_parameters.setValue(ENCODER_CUSTOM_EMBED_COVER_KEY, checkBox_embedcover->isChecked());

    changed = false;

    return success;
}

void customWidget::trigger_changed()
{
    changed = (schemeedit_command->scheme() != p_parameters.value(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, ENCODER_CUSTOM_COMMAND_SCHEME)
               || qlineedit_suffix->text() != p_parameters.value(ENCODER_CUSTOM_SUFFIX_KEY, ENCODER_CUSTOM_SUFFIX)
               || checkBox_tags->isChecked() != p_parameters.value(ENCODER_CUSTOM_TAGS_KEY, ENCODER_CUSTOM_TAGS).toBool()
               || checkBox_embedcover->isChecked() != p_parameters.value(ENCODER_CUSTOM_EMBED_COVER_KEY, ENCODER_CUSTOM_EMBED_COVER).toBool());

    Q_EMIT triggerChanged();
}
