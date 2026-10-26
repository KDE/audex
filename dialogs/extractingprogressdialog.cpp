/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "extractingprogressdialog.h"

#include "dialogs/errordialog.h"
#include "dialogs/logviewdialog.h"
#include "preferences.h"
#include "utils/devicesettings.h"

#include <QDBusConnection>
#include <QDialogButtonBox>
#include <QIcon>
#include <QLocale>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

#include <KColorScheme>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>

using namespace Qt::StringLiterals;

ExtractingProgressDialog::ExtractingProgressDialog(ProfileModel *profile_model,
                                                   Audex::CDInfoModel *cdda_model,
                                                   std::shared_ptr<const Audex::Encoding::EncoderRegistry> encoders,
                                                   const Audex::DriveEntry &drive,
                                                   const QString &driveUdi,
                                                   QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Rip And Encode"));

    mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ExtractingProgressDialog::slotCancel);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);
    ui.result_summary->hide();

    this->profile_model = profile_model;
    this->cdda_model = cdda_model;

    if (!cdda_model || !profile_model) {
        qWarning() << "ExtractingProgressDialog() called with null model pointers";
        Q_ASSERT(cdda_model);
        Q_ASSERT(profile_model);
        return;
    }

    m_encoders = std::move(encoders);
    m_drive = drive;
    m_driveUdi = driveUdi;

    QString title = u"%1 - %2"_s.arg(cdda_model->artist(), cdda_model->album());
    ui.label_header->setText(title);

    p_image_file = profile_model->isCurrentImage();

    if (p_image_file) {
        ui.label_extracting->setText(i18n("Ripping whole CD as image file"));
    } else {
        ui.label_extracting->setText(i18n("Ripping Track 0 of %1", cdda_model->selectedTracks().count()));
    }

    connect(ui.details_button, &QToolButton::clicked, this, &ExtractingProgressDialog::toggle_details);

    finished = false;

    last_sectors = 0;
    speed_ema = 0.0;
    current_track = 0;

    unity_message = QDBusMessage::createSignal("/Audex", "com.canonical.Unity.LauncherEntry", "Update");
}

ExtractingProgressDialog::~ExtractingProgressDialog()
{
    if (m_job) {
        m_job->cancel();
        m_job->wait();
    }
}

int ExtractingProgressDialog::exec()
{
    if (!cdda_model || !profile_model) {
        qWarning() << "ExtractingProgressDialog::exec() called with null model pointers";
        Q_ASSERT(cdda_model);
        Q_ASSERT(profile_model);
        return QDialog::Rejected;
    }

    KConfigGroup grp(KSharedConfig::openConfig(), "ExtractingProgressDialog");

    resize(600, 400);
    m_tracks = cdda_model->selectedTracks();

    m_mapTracks.clear();
    m_mapSectors = 0;
    const Audex::CDInfo mapInfo = cdda_model->cdInfo();
    for (const int number : std::as_const(m_tracks)) {
        const auto entry = mapInfo.entry(number);
        if (!entry)
            continue;
        m_mapTracks.append({entry->sectorCount(), number});
        m_mapSectors += entry->sectorCount();
    }
    ui.discMap->setTracks(m_mapTracks);

    ui.details_button->setArrowType(grp.readEntry("Simple", true) ? Qt::UpArrow : Qt::DownArrow);
    toggle_details();
    show();
    setModal(true);

    RipRequestBuilder builder(profile_model, cdda_model, m_encoders);
    builder.setDrive(m_drive);
    builder.setDriveUdi(m_driveUdi);

    QString error, details;
    QStringList existing;
    if (!builder.validate(&error, &existing, &details)) {
        ErrorDialog::show(this, error, details, i18n("Cannot Start Ripping"));
        return QDialog::Rejected;
    }

    // a drive that does not read accurately makes the rip worthless: say so plainly
    const RipRequestBuilder::StreamWarning stream = builder.streamWarning();
    if (stream.warn) {
        const QString text = stream.measured
            ? i18np(
                  "The drive test found that this drive does not read accurately: after every jump to another position on the disc, "
                  "its reads are off by up to %1 sample (no \"accurate stream\").\n\n"
                  "Audex cannot compensate for this. In secure mode almost every sector will be reported as suspicious and the rip "
                  "takes many hours; in fast mode the audio is shifted and AccurateRip will not confirm it. Neither gives a usable copy.\n\n"
                  "Please use another drive.",
                  "The drive test found that this drive does not read accurately: after every jump to another position on the disc, "
                  "its reads are off by up to %1 samples (no \"accurate stream\").\n\n"
                  "Audex cannot compensate for this. In secure mode almost every sector will be reported as suspicious and the rip "
                  "takes many hours; in fast mode the audio is shifted and AccurateRip will not confirm it. Neither gives a usable copy.\n\n"
                  "Please use another drive.",
                  stream.jitterSamples)
            : i18n(
                  "This drive reports that it does not read accurately (no \"accurate stream\"). If that is true, secure mode will report "
                  "almost every sector as suspicious and fast mode will deliver shifted audio.\n\n"
                  "The drive has not been tested yet. The drive test in the device settings measures it in about two minutes.");
        if (KMessageBox::warningContinueCancel(this,
                                               text,
                                               i18n("Drive Does Not Read Accurately"),
                                               KGuiItem(i18n("Rip Anyway")),
                                               KStandardGuiItem::cancel(),
                                               QString(),
                                               KMessageBox::Notify | KMessageBox::Dangerous)
            != KMessageBox::Continue)
            return QDialog::Rejected;
    }

    if (!existing.isEmpty() && !Preferences::overwriteExistingFiles()) {
        if (KMessageBox::warningTwoActionsList(this,
                                               i18n("The following files already exist. Do you want to overwrite them?"),
                                               existing,
                                               i18n("Files Already Exist"),
                                               KStandardGuiItem::overwrite(),
                                               KStandardGuiItem::cancel())
            != KMessageBox::PrimaryAction) {
            return QDialog::Rejected;
        }
    }

    const RipRequestBuilder::DiskSpace space = builder.diskSpace();
    if (!space.enough()) {
        const QLocale locale;
        if (KMessageBox::warningContinueCancel(this,
                                               i18n("The output folder %1 has %2 free, but the rip needs about %3. Start it anyway?",
                                                    space.folder,
                                                    locale.formattedDataSize(space.available),
                                                    locale.formattedDataSize(space.needed)),
                                               i18n("Not Enough Disk Space"),
                                               KGuiItem(i18n("Start Anyway")))
            != KMessageBox::Continue)
            return QDialog::Rejected;
    }

    m_plan = builder.plan();

    m_job = new Audex::RipJob(builder.request(), this);
    connect(m_job, &Audex::RipJob::progress, this, &ExtractingProgressDialog::onProgress);
    connect(m_job, &Audex::RipJob::trackStatus, this, &ExtractingProgressDialog::onTrackStatus);
    connect(m_job, &Audex::RipJob::cdgProgress, this, &ExtractingProgressDialog::onCdgProgress);
    connect(m_job, &Audex::RipJob::message, this, &ExtractingProgressDialog::onMessage);
    connect(m_job, &Audex::RipJob::finished, this, &ExtractingProgressDialog::onFinished);
    speed_timer.start();
    m_job->start();

    int rv = QDialog::exec();

    grp.writeEntry("Simple", (Qt::DownArrow == ui.details_button->arrowType()));

    return rv;
}

void ExtractingProgressDialog::toggle_details()
{
    const bool expand = (Qt::UpArrow == ui.details_button->arrowType());
    ui.details_button->setArrowType(expand ? Qt::DownArrow : Qt::UpArrow);
    ui.details->setVisible(expand);

    // the spacer only keeps the collapsed content at the top
    ui.verticalSpacer->changeSize(20, 0, QSizePolicy::Minimum, expand ? QSizePolicy::Minimum : QSizePolicy::Expanding);

    // update the minimum size now, otherwise resize() is clamped to the old one;
    // the inner layout goes first, activating it propagates the change to mainLayout
    ui.gridLayout_2->invalidate();
    ui.gridLayout_2->activate();
    mainLayout->activate();
    resize(width(), expand ? 400 : minimumSizeHint().height());
}

void ExtractingProgressDialog::slotCancel()
{
    cancel();
}

void ExtractingProgressDialog::slotClose()
{
    close();
}

void ExtractingProgressDialog::slotLog()
{
    open_log_view_dialog();
}

void ExtractingProgressDialog::reject()
{
    // Esc and the window's close button end up here
    if (finished)
        QDialog::reject();
    else
        cancel();
}

void ExtractingProgressDialog::cancel()
{
    if (finished) {
        close();
        return;
    }
    if (m_cancelRequested)
        return;

    const auto answer = KMessageBox::warningTwoActions(this,
                                                       i18n("Do you really want to cancel the extraction?"),
                                                       i18n("Cancel Extraction"),
                                                       KGuiItem(i18n("Cancel Extraction"), QIcon::fromTheme(QStringLiteral("process-stop"))),
                                                       KStandardGuiItem::cont());
    // the rip may have finished while the question was open
    if (answer != KMessageBox::PrimaryAction || finished)
        return;

    m_cancelRequested = true;
    if (cancelButton)
        cancelButton->setEnabled(false);
    show_info(i18n("Canceling..."));
    if (m_job)
        m_job->cancel();
}

void ExtractingProgressDialog::onProgress(qint64 doneSectors, qint64 totalSectors, int trackNumber, qint64 trackSectors)
{
    if (!cdda_model) {
        qWarning() << "ExtractingProgressDialog::onProgress() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    m_percent = totalSectors > 0 ? int(doneSectors * 100 / totalSectors) : 0;
    ui.discMap->setTrackProgress(trackNumber, trackSectors);
    ui.discMap->setPercent(m_percent);

    const int pos = qMax(1, m_tracks.indexOf(trackNumber) + 1);
    current_track = pos;

    if (p_image_file) {
        ui.label_extracting->setText(i18n("Ripping whole CD as image file..."));
    } else if (m_rereading.contains(trackNumber)) {
        ui.label_extracting->setText(i18n("Reading track %1 again in secure mode...", pos));
    } else {
        ui.label_extracting->setText((1 == m_tracks.count()) ? i18n("Ripping track...") : i18n("Ripping track %1 of %2...", pos, m_tracks.count()));
    }

    // speed: sectors per second / 75 sectors per second at 1x
    if (speed_timer.elapsed() >= 500) {
        const double sps = (double)(doneSectors - last_sectors) / ((double)speed_timer.elapsed() / 1000.0);
        const double speed = sps / 75.0;
        speed_ema = (speed_ema <= 0.0) ? speed : (0.3 * speed + 0.7 * speed_ema);
        last_sectors = doneSectors;
        speed_timer.restart();
        ui.label_speed_extracting->setText(i18n("<i>Speed: %1×</i>", QLocale().toString(speed_ema, 'f', 2)));
        // the read position on the disc: stays within the disc and follows
        // the drive back when a track is read again
        const QString sector = i18n("Sector %1 of %2", QLocale().toString(discPosition(trackNumber, trackSectors)), QLocale().toString(m_mapSectors));
        const QString errors = i18np("%1 read error", "%1 read errors", m_errorCount);
        if (m_rereading.contains(trackNumber))
            ui.label_telemetry->setText(i18nc("sector, re-read, read errors", "%1 · re-read %2 of %3 · %4", sector, m_rereadIndex, m_rereadCount, errors));
        else
            ui.label_telemetry->setText(i18nc("sector, read errors", "%1 · %2", sector, errors));
    }

    update_unity();
}

void ExtractingProgressDialog::onTrackStatus(int trackNumber, int status)
{
    using Audex::Rip::SegmentStatus;
    using State = DiscMapWidget::TrackState;

    State state = State::Done;
    switch (static_cast<SegmentStatus>(status)) {
    case SegmentStatus::Confirmed:
        state = State::Confirmed;
        break;
    case SegmentStatus::Unconfirmed:
        state = State::Unconfirmed;
        break;
    case SegmentStatus::Rereading:
        state = State::Rereading;
        break;
    case SegmentStatus::Done:
        state = State::Done;
        break;
    case SegmentStatus::Suspicious:
        state = State::Suspicious;
        break;
    }
    if (state == State::Unconfirmed)
        ++m_rereadCount;
    if (state == State::Rereading) {
        ++m_rereadIndex;
        m_rereading.insert(trackNumber);
    } else {
        m_rereading.remove(trackNumber);
    }
    ui.discMap->setTrackState(trackNumber, state);
}

// After the audio: the sub-channel of the whole range, read twice. The disc
// map keeps the track colors and shows the pass as a line with a band.
void ExtractingProgressDialog::onCdgProgress(int pass, qint64 doneSectors, qint64 totalSectors, int trackNumber, qint64 trackSectors)
{
    const int percent = totalSectors > 0 ? int(doneSectors * 100 / totalSectors) : 0;
    if (pass != m_cdgPass) {
        // a new pass: the speed is measured from here, the percentage of the
        // phase starts at 0 (it grows by the sectors read again: never back)
        if (m_cdgPass == 0)
            m_percent = 0;
        m_cdgPass = pass;
        last_sectors = doneSectors;
        speed_timer.restart();
        speed_ema = 0.0;
        if (pass == 3)
            ui.label_speed_extracting->clear(); // single sectors: no meaningful speed
    }
    m_percent = std::max(m_percent, percent);
    ui.discMap->setPercent(m_percent);

    if (totalSectors > 0 && doneSectors >= totalSectors) {
        ui.discMap->setScanPosition(-1, 0);
        ui.label_extracting->setText(i18n("Finishing..."));
        ui.label_speed_extracting->clear();
        update_unity();
        return;
    }

    ui.discMap->setScanPosition(trackNumber, trackSectors, i18nc("caption before the percentage on the disc map", "CD+G"));
    if (pass == 3)
        ui.label_extracting->setText(i18n("Reading differing sectors of the CD+G graphics again..."));
    else
        ui.label_extracting->setText(i18n("Reading the CD+G graphics, pass %1 of 2...", pass));

    if (speed_timer.elapsed() >= 500) {
        if (pass < 3) {
            const double sps = (double)(doneSectors - last_sectors) / ((double)speed_timer.elapsed() / 1000.0);
            const double speed = sps / 75.0;
            speed_ema = (speed_ema <= 0.0) ? speed : (0.3 * speed + 0.7 * speed_ema);
            ui.label_speed_extracting->setText(i18n("<i>Speed: %1×</i>", QLocale().toString(speed_ema, 'f', 2)));
        }
        last_sectors = doneSectors;
        speed_timer.restart();
        const QString sector = i18n("Sector %1 of %2", QLocale().toString(discPosition(trackNumber, trackSectors)), QLocale().toString(m_mapSectors));
        const QString errors = i18np("%1 read error", "%1 read errors", m_errorCount);
        ui.label_telemetry->setText(i18nc("sector, read errors", "%1 · %2", sector, errors));
    }

    update_unity();
}

void ExtractingProgressDialog::onMessage(int level, const QString &text)
{
    switch (static_cast<Audex::Rip::LogLevel>(level)) {
    case Audex::Rip::LogLevel::Warning:
        show_warning(text);
        ui.discMap->addErrorMark();
        ++m_errorCount;
        break;
    case Audex::Rip::LogLevel::Error:
        show_error(text, QString());
        ui.discMap->addErrorMark();
        ++m_errorCount;
        break;
    case Audex::Rip::LogLevel::Info:
        show_info(text);
        break;
    default:
        break; // Debug
    }
}

void ExtractingProgressDialog::onFinished(const Audex::RipSummary &summary)
{
    if (!cdda_model) {
        qWarning() << "ExtractingProgressDialog::onFinished() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    m_summary = summary;
    ui.discMap->setScanPosition(-1, 0); // a canceled CD+G pass
    if (summary.measuredCacheDefeatReads > 0)
        DeviceSettings::setCacheDefeatReads(m_driveUdi, summary.measuredCacheDefeatReads);

    PostProcessResult post;
    if (summary.completed && !summary.canceled) {
        post = RipRequestBuilder::runPostProcess(m_plan, summary, cdda_model->cdInfo(), m_tracks, this, [this](int level, const QString &text) {
            onMessage(level, text);
        });
    }

    if (summary.canceled)
        show_info(i18n("Ripping canceled."));
    else if (!summary.completed)
        show_error(summary.error.isEmpty() ? i18n("Ripping failed.") : summary.error, QString());

    if (summary.suspiciousPositions > 0)
        show_warning(i18np("%1 position could not be read reliably; the audio may contain errors.",
                           "%1 positions could not be read reliably; the audio may contain errors.",
                           summary.suspiciousPositions));
    if (summary.accurateRipMismatches > 0)
        show_warning(
            i18np("%1 track does not match the AccurateRip database.", "%1 tracks do not match the AccurateRip database.", summary.accurateRipMismatches));
    if (summary.ctdbMismatches > 0)
        show_warning(i18np("%1 track does not match the CUETools database.", "%1 tracks do not match the CUETools database.", summary.ctdbMismatches));

    conclusion(post);
}

void ExtractingProgressDialog::conclusion(const PostProcessResult &post)
{
    using Audex::JobStatus;

    if (post.failedTasks > 0 && m_summary.output.status == JobStatus::Success)
        m_summary.output.status = JobStatus::Warning;

    const auto describe = [&](JobStatus status, const QString &success, const QString &warning, const QString &failure) {
        switch (status) {
        case JobStatus::Success:
            return success;
        case JobStatus::Warning:
            return warning;
        case JobStatus::Failed:
            return failure;
        case JobStatus::Canceled:
            return i18n("Canceled.");
        case JobStatus::Skipped:
            return i18n("Not performed.");
        }
        return QString();
    };
    const QString extraction = describe(
        m_summary.extraction.status,
        i18np("%1 track extracted without read errors.", "%1 tracks extracted without read errors.", m_summary.tracks),
        m_summary.suspiciousPositions > 0
            ? i18np("Extracted with %1 unreliable position.", "Extracted with %1 unreliable positions.", m_summary.suspiciousPositions)
            : i18n("Extracted with warnings. See rip log."),
        m_summary.error.isEmpty() ? i18n("Extraction failed.") : m_summary.error);

    QString verification;
    if (m_summary.verification.status == JobStatus::Skipped) {
        verification = m_summary.verificationRequested ? i18n("Not performed.") : i18n("Not requested.");
    } else if (m_summary.verification.status == JobStatus::Failed) {
        verification = i18n("The repaired image could not be verified.");
    } else if (m_summary.verification.status == JobStatus::Success) {
        verification = i18np("%1 track confirmed by AccurateRip or CTDB.", "%1 tracks confirmed by AccurateRip or CTDB.", m_summary.verifiedTracks);
    } else {
        QStringList details;
        if (m_summary.verifiedTracks < m_summary.verifiableTracks)
            details << i18n("%1 of %2 tracks confirmed.", m_summary.verifiedTracks, m_summary.verifiableTracks);
        if (m_summary.accurateRipMismatches > 0 || m_summary.ctdbMismatches > 0)
            details << i18n("Database mismatches detected.");
        if (!m_summary.accurateRipError.isEmpty() || !m_summary.ctdbError.isEmpty())
            details << i18n("Database lookup failed.");
        if (details.isEmpty())
            details << i18n("Verification incomplete. See rip log.");
        verification = details.join(u" "_s);
    }

    QString output;
    if (post.failedTasks > 0 && m_summary.output.status != JobStatus::Failed) {
        output = i18np("%1 optional output task failed.", "%1 optional output tasks failed.", post.failedTasks);
    } else {
        output = describe(m_summary.output.status,
                          i18n("Audio and requested files saved."),
                          i18n("Audio saved; some output operations failed. See rip log."),
                          m_summary.error.isEmpty() ? i18n("Audio output failed.") : m_summary.error);
    }
    if (post.hookPending)
        output += u" "_s + i18n("External command running separately; result pending.");

    const auto setResult = [](QLabel *icon, QLabel *label, JobStatus status, const QString &heading, const QString &message) {
        QString name;
        switch (status) {
        case JobStatus::Success:
            name = u"dialog-ok-apply"_s;
            break;
        case JobStatus::Warning:
            name = u"dialog-warning"_s;
            break;
        case JobStatus::Failed:
        case JobStatus::Canceled:
            name = u"dialog-error"_s;
            break;
        case JobStatus::Skipped:
            name = u"dialog-information"_s;
            break;
        }
        icon->setPixmap(QIcon::fromTheme(name).pixmap(16, 16));
        label->setText(u"<b>%1</b> %2"_s.arg(heading.toHtmlEscaped(), message.toHtmlEscaped()));
    };
    setResult(ui.extraction_icon, ui.extraction_result, m_summary.extraction.status, i18n("Extraction:"), extraction);
    setResult(ui.verification_icon, ui.verification_result, m_summary.verification.status, i18n("Verification:"), verification);
    setResult(ui.output_icon, ui.output_result, m_summary.output.status, i18n("Output:"), output);
    ui.result_summary->show();

    const JobStatus status = Audex::overallStatus(m_summary.extraction, m_summary.verification, m_summary.output);
    const bool successful = status == JobStatus::Success || status == JobStatus::Warning;
    const bool warnings = status == JobStatus::Warning;

    // Remove the cancel button
    buttonBox->clear();
    // Add the new close button
    buttonBox->addButton(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ExtractingProgressDialog::slotClose);

    finished = true;

    update_unity();

    QPalette pal(ui.label_extracting->palette());
    KColorScheme kcs(QPalette::Active);
    if (successful && !warnings) {
        QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme("dialog-ok-apply"), i18n("Extraction, verification and output finished successfully."));
        ui.klistwidget->addItem(item);
        ui.klistwidget->scrollToItem(item);
        pal.setBrush(QPalette::Text, kcs.foreground(KColorScheme::PositiveText));
        ui.label_extracting->setText("<font style=\"font-weight:bold;\">" + i18n("Finished!") + "</font>");

    } else if (successful) {
        QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme("dialog-warning"), i18n("Finished with warnings. Please check the rip log."));
        ui.klistwidget->addItem(item);
        ui.klistwidget->scrollToItem(item);
        pal.setBrush(QPalette::Text, kcs.foreground(KColorScheme::NeutralText));
        const QString color = kcs.foreground(KColorScheme::NeutralText).color().name();
        const QString text = u"<font style=\"color:%1;font-weight:bold;\">%2</font>"_s.arg(color, i18n("Finished with warnings"));
        ui.label_extracting->setText(text);

    } else {
        QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme("dialog-cancel"),
                                                     status == JobStatus::Canceled ? i18n("Ripping canceled.") : i18n("At least one job failed."));
        pal.setBrush(QPalette::Text, kcs.foreground(KColorScheme::NegativeText));
        ui.klistwidget->addItem(item);
        ui.klistwidget->scrollToItem(item);
        ui.label_extracting->setText("<font style=\"color:red;font-weight:bold;\">"
                                     + (status == JobStatus::Canceled ? i18n("Canceled") : i18n("Failed!")) + "</font>");
    }

    if (m_summary.report.count() > 0) {
        auto *logButton = new QPushButton();
        logButton->setText(i18n("Show Rip Log..."));
        logButton->setIcon(QIcon::fromTheme(u"media-optical"_s));
        buttonBox->addButton(logButton, QDialogButtonBox::HelpRole);
        connect(logButton, &QPushButton::clicked, this, &ExtractingProgressDialog::slotLog);
    }
    ui.label_speed_extracting->setEnabled(false);
    ui.label_extracting->setPalette(pal);
}

void ExtractingProgressDialog::show_info(const QString &message)
{
    QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme("dialog-information"), message);
    ui.klistwidget->addItem(item);
    ui.klistwidget->scrollToItem(item);
}

void ExtractingProgressDialog::show_warning(const QString &message)
{
    QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme("dialog-warning"), message);
    ui.klistwidget->addItem(item);
    ui.klistwidget->scrollToItem(item);
}

void ExtractingProgressDialog::show_error(const QString &message, const QString &details)
{
    QListWidgetItem *item;
    if (details.isEmpty()) {
        item = new QListWidgetItem(QIcon::fromTheme("dialog-error"), QString("%1").arg(message));
    } else {
        item = new QListWidgetItem(QIcon::fromTheme("dialog-error"), QString("%1 (%2)").arg(message, details));
    }
    ui.klistwidget->addItem(item);
    ui.klistwidget->scrollToItem(item);
}

void ExtractingProgressDialog::open_log_view_dialog()
{
    LogViewDialog logViewDialog(m_summary.report, i18n("Ripping log"), this);
    logViewDialog.exec();
}

qint64 ExtractingProgressDialog::discPosition(int trackNumber, qint64 trackSectors) const
{
    qint64 offset = 0;
    for (const auto &track : m_mapTracks) {
        if (track.second == trackNumber)
            return offset + std::clamp<qint64>(trackSectors, 0, track.first);
        offset += track.first;
    }
    return 0;
}

void ExtractingProgressDialog::update_unity()
{
    QList<QVariant> args;
    const int progress = m_percent;
    bool show_progress = progress > -1 && progress < 100 && !finished;
    QMap<QString, QVariant> props;
    props["count-visible"] = current_track > 0 && !finished;
    props["count"] = current_track;
    props["progress-visible"] = show_progress;
    props["progress"] = show_progress ? (double)(progress / 100.0) : 0.0;
    args.append("application://org.kde.audex.desktop");
    args.append(props);
    unity_message.setArguments(args);
    QDBusConnection::sessionBus().send(unity_message);
}
