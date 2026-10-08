/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ui_extractingprogresswidgetUI.h"

#include "models/cdinfomodel.h"
#include "models/profilemodel.h"
#include "utils/riprequestbuilder.h"

#include <QDBusMessage>
#include <QDialog>
#include <QPointer>
#include <QSet>

#include <memory>

class QVBoxLayout;
class QDialogButtonBox;

class ExtractingProgressDialog : public QDialog
{
    Q_OBJECT

public:
    ExtractingProgressDialog(ProfileModel *profile_model,
                             Audex::CDInfoModel *cdda_model,
                             std::shared_ptr<const Audex::Encoding::EncoderRegistry> encoders,
                             const Audex::DriveEntry &drive,
                             const QString &driveUdi,
                             QWidget *parent = nullptr);
    ~ExtractingProgressDialog() override;

public Q_SLOTS:
    int exec() override;
    void reject() override;

private Q_SLOTS:
    void toggle_details();
    void cancel();

    void slotCancel();
    void slotClose();
    void slotLog();

    void onProgress(qint64 doneSectors, qint64 totalSectors, int trackNumber, qint64 trackSectors);
    void onTrackStatus(int trackNumber, int status);
    void onCdgProgress(int pass, qint64 doneSectors, qint64 totalSectors, int trackNumber, qint64 trackSectors);
    void onMessage(int level, const QString &text);
    void onFinished(const Audex::RipSummary &summary);

    void conclusion(bool successful, bool warnings);

    void show_info(const QString &message);
    void show_warning(const QString &message);
    void show_error(const QString &message, const QString &details);

private:
    Ui::ExtractingProgressWidgetUI ui;

    QVBoxLayout *mainLayout = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
    QPointer<QPushButton> cancelButton; // deleted by conclusion()

    void open_log_view_dialog();
    qint64 discPosition(int trackNumber, qint64 trackSectors) const; // read position on the disc map
    void update_unity();

    QPointer<ProfileModel> profile_model;
    QPointer<Audex::CDInfoModel> cdda_model;
    std::shared_ptr<const Audex::Encoding::EncoderRegistry> m_encoders;
    Audex::DriveEntry m_drive;
    QString m_driveUdi;

    QPointer<Audex::RipJob> m_job;
    Audex::RipSummary m_summary;
    PostProcessPlan m_plan;
    QList<int> m_tracks;

    bool finished;
    bool m_cancelRequested = false;

    bool p_image_file;

    // speed measurement (sectors per second, converted to x-factor)
    QElapsedTimer speed_timer;
    qint64 last_sectors;
    double speed_ema;
    int current_track;
    int m_errorCount = 0;
    int m_percent = 0; // for the launcher progress, the bars are gone
    QSet<int> m_rereading; // tracks being read again in secure mode
    int m_rereadCount = 0; // tracks discarded for a secure re-read
    int m_rereadIndex = 0; // number of the running re-read
    QList<QPair<qint64, int>> m_mapTracks; // (sectors, track number), as on the disc map
    qint64 m_mapSectors = 0;
    int m_cdgPass = 0; // CD+G after the audio: running pass, 0 = not (yet)

    QDBusMessage unity_message;
};
