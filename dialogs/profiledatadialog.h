/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

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
#include "dialogs/profiledatacoverdialog.h"
#include "dialogs/profiledatacuesheetdialog.h"
#include "dialogs/profiledatahashlistdialog.h"
#include "dialogs/profiledatainfodialog.h"
#include "dialogs/profiledatalogfiledialog.h"
#include "dialogs/profiledataplaylistdialog.h"
#include "dialogs/profiledatasinglefiledialog.h"
#include "dialogs/schemewizarddialog.h"

#include "ui_profiledatawidgetUI.h"

namespace Audex
{

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
    void enable_settings_logfile(bool enabled);
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
    void logfile_settings();
    void singlefile_settings();

    void slotAccepted();
    void slotApplied();
    void slotRejected();

private:
    Ui::ProfileDataWidgetUI ui;
    ProfileModel *profile_model;

    QPointer<QPushButton> applyButton;

    int profile_row;
    bool new_profile_mode;

    QPointer<lameWidget> lame_widget;
    Parameters lame_parameters;
    QPointer<oggencWidget> oggenc_widget;
    Parameters oggenc_parameters;
    QPointer<opusencWidget> opusenc_widget;
    Parameters opusenc_parameters;
    QPointer<flacWidget> flac_widget;
    Parameters flac_parameters;
    QPointer<faacWidget> faac_widget;
    Parameters faac_parameters;
    QPointer<waveWidget> wave_widget;
    Parameters wave_parameters;
    QPointer<customWidget> custom_widget;
    Parameters custom_parameters;
    void set_encoder_widget(const EncoderAssistant::Encoder encoder);

    bool cover_scale;
    QSize cover_size;
    QString cover_format;
    QString cover_scheme;

    QString playlist_format;
    QString playlist_scheme;
    bool playlist_abs_file_path;
    bool playlist_utf8;

    QStringList infofile_text;
    QString infofile_scheme;
    QString infofile_suffix;

    QString hashlist_format;
    QString hashlist_scheme;

    QString cuesheet_scheme;
    bool cuesheet_write_mcn_and_isrc;

    QString logfile_scheme;
    bool logfile_write_timestamps;

    QString singlefile_scheme;

    bool save();

    Error error;
};

}
