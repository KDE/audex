/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractButton>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <KColorScheme>
#include <KConfigGroup>
#include <KMessageBox>

#include "datatypes/toc.h"
#include "models/cddamodel.h"
#include "models/profilemodel.h"
#include "utils/audex.h"

#include "logviewdialog.h"

#include "ui_extractingprogresswidgetUI.h"

namespace Audex
{

class ExtractingProgressDialog : public QDialog
{
    Q_OBJECT

public:
    ExtractingProgressDialog(QPointer<ProfileModel> profile_model,
                             const Audex::CDDA &cdda,
                             const Audex::TracknumberSet &selectedTracks,
                             const QByteArray &blockDevice,
                             QWidget *parent = nullptr);

public Q_SLOTS:
    int exec() override;

private Q_SLOTS:
    void toggle_details();
    void cancel();

    void slotCancel();
    void slotClose();
    void slotEncoderLog();
    void slotExtractLog();

    void show_changed_extract_track(int no, int total, const QString &artist, const QString &title);
    void show_changed_encode_track(int no, int total, const QString &filename);

    void show_progress_extract_track(int percent);
    void show_progress_extract_overall(int percent);
    void show_progress_encode_track(int percent);
    void show_progress_encode_overall(int percent);

    void show_speed_encode(double speed);
    void show_speed_extract(double speed);

    void conclusion(bool successful);

    void show_info(const QString &message);
    void show_warning(const QString &message);
    void show_error(const QString &message, const QString &details);

    void ask_timeout();

private:
    QVBoxLayout *mainLayout;
    QDialogButtonBox *buttonBox;
    QPushButton *cancelButton;

    void calc_overall_progress();
    void open_encoder_log_view_dialog();
    void open_extract_log_view_dialog();
    void update_unity();

private:
    Ui::ExtractingProgressWidgetUI ui;

    QPointer<Audex::AudexRipManager> audex_rip_manager;
    QPointer<ProfileModel> profile_model;
    Audex::CDDA cdda;

    Audex::TracknumberSet selected_tracks;

    bool finished;

    bool progressbar_np_flag;
    int current_encode_overall;
    int current_extract_overall;
    unsigned int current_track;

    bool p_single_file;

    QDBusMessage unity_message;
};

}
