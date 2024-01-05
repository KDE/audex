/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef EXTRACTINGPROGRESSDIALOG_H
#define EXTRACTINGPROGRESSDIALOG_H

#include <QAbstractButton>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <KColorScheme>
#include <KConfigGroup>
#include <KMessageBox>

#include "core/audex.h"
#include "models/cddamodel.h"
#include "models/profilemodel.h"

#include "protocolviewdialog.h"

#include "ui_extractingprogresswidgetUI.h"

class ExtractingProgressDialog : public QDialog
{
    Q_OBJECT

public:
    ExtractingProgressDialog(ProfileModel *profile_model, CDDAModel *cdda_model, QWidget *parent = nullptr);
    ~ExtractingProgressDialog() override;

public Q_SLOTS:
    int exec() override;

private Q_SLOTS:
    void toggle_details();
    void cancel();

    void slotCancel();
    void slotClose();
    void slotEncoderProtocol();
    void slotExtractProtocol();

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
    void open_encoder_protocol_view_dialog();
    void open_extract_protocol_view_dialog();
    void update_unity();

private:
    Ui::ExtractingProgressWidgetUI ui;

    Audex *audex;
    ProfileModel *profile_model;
    CDDAModel *cdda_model;

    bool finished;

    bool progressbar_np_flag;
    int current_encode_overall;
    int current_extract_overall;
    unsigned int current_track;

    bool p_single_file;

    QDBusMessage unity_message;
};

#endif
