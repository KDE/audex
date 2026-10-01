/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_profiledatawidgetUI.h"

#include "models/profilemodel.h"
#include "widgets/customwidget.h"
#include "widgets/flacwidget.h"
#include "widgets/lamewidget.h"
#include "widgets/opusencwidget.h"
#include "widgets/wavewidget.h"

#include <QDialog>
#include <QPointer>

class KMessageWidget;

class ProfileDataDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataDialog(ProfileModel *profileModel, const int profileRow, QWidget *parent = nullptr);

private Q_SLOTS:
    void set_encoder(const int encoder);
    void set_encoder_by_combobox(const int index);
    void set_output_by_combobox(const int index);
    void trigger_changed();

    void enable_settings_cover(bool enabled);
    void enable_settings_playlist(bool enabled);
    void enable_settings_logfile(bool enabled);
    void enable_settings_hook(bool enabled);
    void enable_settings_cue(bool enabled);
    void enable_settings_ctdb_repair(bool enabled);

    void scheme_wizard();
    void image_scheme_wizard();
    void cue_scheme_wizard();

    void cover_settings();
    void playlist_settings();
    void logfile_settings();
    void hook_settings();

    void slotAccepted();
    void slotApplied();
    void slotRejected();

private:
    Ui::ProfileDataWidgetUI ui;
    QPointer<ProfileModel> profile_model;

    QPushButton *applyButton = nullptr;

    int profile_row;
    bool new_profile_mode;

    // The encoder widgets each hold their own copy of the parameters; save()
    // reads them back out. Nothing here points into another object.
    lameWidget *lame_widget = nullptr;
    opusencWidget *opusenc_widget = nullptr;
    flacWidget *flac_widget = nullptr;
    waveWidget *wave_widget = nullptr;
    customWidget *custom_widget = nullptr;
    void set_encoder_widget(const EncoderAssistant::Encoder encoder);

    KMessageWidget *encoder_message = nullptr;
    EncoderAssistant::Encoder selected_encoder() const;
    bool image_output() const;
    void set_output(const int output);
    void update_encoder_items();
    void update_encoder_message();

    bool save();

    Error error;
};
