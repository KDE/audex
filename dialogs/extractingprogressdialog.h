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
#include <QElapsedTimer>
#include <QPointer>
#include <QSet>
#include <QTimer>

#include <memory>

class KBusyIndicatorWidget;
class QDialogButtonBox;
class QVBoxLayout;

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

    // the rip ran and read the disc completely (not canceled, not failed)
    bool ripCompleted() const
    {
        return m_summary.completed && !m_summary.canceled;
    }

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

    void conclusion(const PostProcessResult &post);

    void show_info(const QString &message);
    void show_warning(const QString &message);
    void show_error(const QString &message, const QString &details);

private:
    Ui::ExtractingProgressWidgetUI ui;

    QVBoxLayout *mainLayout = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
    QPointer<QPushButton> cancelButton; // deleted by conclusion()
    KBusyIndicatorWidget *m_busy = nullptr; // in the icon cell of the extraction row while it runs

    // The three rows that stay in place from start to finish: extraction,
    // verification, output. Pending rows are greyed, the running one spins,
    // a finished one shows the outcome.
    enum class Stage { Extraction, Verification, Output };
    enum class StageState { Pending, Running, Success, Warning, Failed, Canceled, Skipped };
    void set_stage(Stage stage, StageState state, const QString &text);
    static StageState stageState(Audex::JobStatus status);
    void update_extraction_row(); // m_reading and the speed
    void update_verification_row(); // confirmed tracks so far
    void update_time();
    QString outputFolder() const; // where the audio files go

    void open_log_view_dialog();
    void open_output_folder();
    qint64 discPosition(int trackNumber, qint64 trackSectors) const; // read position on the disc map
    void update_unity();

    ProfileModel *profile_model = nullptr;
    Audex::CDInfoModel *cdda_model = nullptr;
    std::shared_ptr<const Audex::Encoding::EncoderRegistry> m_encoders;
    Audex::DriveEntry m_drive;
    QString m_driveUdi;

    QPointer<Audex::RipJob> m_job;
    Audex::RipSummary m_summary;
    PostProcessPlan m_plan;
    Audex::CDInfo m_disc; // as the rip was started
    QList<int> m_tracks;

    bool finished;
    bool m_cancelRequested = false;

    bool p_image_file;

    // speed measurement (sectors per second, converted to x-factor)
    QElapsedTimer speed_timer;
    qint64 last_sectors;
    double speed_ema;
    QString m_reading; // what the extraction row says is being read
    QElapsedTimer m_clock; // since the job started
    QTimer m_clockTimer;
    bool m_verificationRequested = false;
    QSet<int> m_finishedTracks; // tracks with a final state on the disc map
    QSet<int> m_confirmedTracks; // of these, confirmed by a database
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
