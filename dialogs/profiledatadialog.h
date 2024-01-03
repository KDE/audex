/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PROFILEDATADIALOG_H
#define PROFILEDATADIALOG_H

#include <QStackedWidget>
#include <QWidget>

#include <QDialog>
#include <QPushButton>

#include "models/profilemodel.h"
#include "utils/encoderassistant.h"

#include "widgets/customwidget.h"
#include "widgets/faacwidget.h"
#include "widgets/flacwidget.h"
#include "widgets/lamewidget.h"
#include "widgets/oggencwidget.h"
#include "widgets/opusencwidget.h"
#include "widgets/wavewidget.h"

#include "dialogs/errordialog.h"
#include "dialogs/schemewizarddialog.h"
#include "dialogs/profiledatacoverdialog.h"
#include "dialogs/profiledatacuesheetdialog.h"
#include "dialogs/profiledatahashlistdialog.h"
#include "dialogs/profiledatainfodialog.h"
#include "dialogs/profiledataplaylistdialog.h"
#include "dialogs/profiledatasinglefiledialog.h"

#include "ui_profiledatawidgetUI.h"

class ProfileDataDialog : public QDialog
{
    Q_OBJECT

public:
    ProfileDataDialog(ProfileModel *profileModel, const int profileRow, QWidget *parent = nullptr);
    ~ProfileDataDialog() override;

private Q_SLOTS:
    void set_encoder(const int encoder);
    void set_encoder_by_combobox(const int index);
    void trigger_changed();

    void enable_settings_cover(bool enabled);
    void enable_settings_playlist(bool enabled);
    void enable_settings_info(bool enabled);
    void enable_settings_hashlist(bool enabled);
    void enable_settings_cuesheet(bool enabled);
    void enable_settings_singlefile(bool enabled);

    void disable_playlist(bool disabled);

    void enable_filenames(bool enabled);
    void disable_filenames(bool disabled);

    void scheme_wizard();

    void cover_settings();
    void playlist_settings();
    void info_settings();
    void hashlist_settings();
    void cuesheet_settings();
    void singlefile_settings();

    void slotAccepted();
    void slotApplied();

private:
    Ui::ProfileDataWidgetUI ui;
    ProfileModel *profile_model;
    QPushButton *applyButton;

    int profile_row;

    lameWidget *lame_widget;
    Parameters lame_parameters;
    oggencWidget *oggenc_widget;
    Parameters oggenc_parameters;
    opusencWidget *opusenc_widget;
    Parameters opusenc_parameters;
    flacWidget *flac_widget;
    Parameters flac_parameters;
    faacWidget *faac_widget;
    Parameters faac_parameters;
    waveWidget *wave_widget;
    Parameters wave_parameters;
    customWidget *custom_widget;
    Parameters custom_parameters;
    void set_encoder_widget(const EncoderAssistant::Encoder encoder);

    bool pdcd_scale;
    QSize pdcd_size;
    QString pdcd_format;
    QString pdcd_scheme;

    QString pdpd_format;
    QString pdpd_scheme;
    bool pdpd_abs_file_path;
    bool pdpd_utf8;

    QStringList pdid_text;
    QString pdid_scheme;
    QString pdid_suffix;

    QString pdhd_format;
    QString pdhd_scheme;

    QString pdud_scheme;

    QString pdsd_scheme;

    bool save();

    Error error;
};

#endif
