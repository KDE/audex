/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "driveassistantdialog.h"

#include "online/accuraterip.h"

#include <KLocalizedString>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QIcon>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace
{

// The engine names its checks in English (as in the rip log); these are the
// labels for the dialog.
QString checkLabel(const Audex::Rip::FeatureCheck &check)
{
    if (check.id == u"burst")
        return i18n("Read command size");
    if (check.id == u"cache")
        return i18n("Audio cache");
    if (check.id == u"stream")
        return i18n("Accurate stream");
    if (check.id == u"c2")
        return i18n("C2 error pointers");
    if (check.id == u"overread")
        return i18n("Overread");
    if (check.id == u"subchannel")
        return i18n("Q sub-channel");
    if (check.id == u"offset")
        return i18n("Read offset");
    if (check.id == u"rw")
        return i18n("R-W sub-channel (CD+G)");
    return check.name;
}

QIcon verdictIcon(Audex::Rip::Feature verdict)
{
    switch (verdict) {
    case Audex::Rip::Feature::Yes:
        return QIcon::fromTheme(u"dialog-ok"_s);
    case Audex::Rip::Feature::No:
        return QIcon::fromTheme(u"dialog-cancel"_s);
    case Audex::Rip::Feature::Unknown:
        break;
    }
    return QIcon::fromTheme(u"dialog-information"_s);
}

}

DriveAssistantDialog::DriveAssistantDialog(const Audex::DriveEntry &drive, QWidget *parent)
    : QDialog(parent)
    , m_drive(drive)
{
    setWindowTitle(i18n("Drive Assistant"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    auto *widget = new QWidget(this);
    ui.setupUi(widget);
    mainLayout->addWidget(widget);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close);
    m_startButton = buttonBox->addButton(i18n("Start Test"), QDialogButtonBox::ActionRole);
    m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
    m_applyButton->setEnabled(false);
    m_applyButton->setToolTip(i18n("Stores the results for this drive and fills in the settings that follow from them."));
    mainLayout->addWidget(buttonBox);

    connect(m_startButton, &QPushButton::clicked, this, &DriveAssistantDialog::startOrStop);
    connect(m_applyButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(&m_watcher, &QFutureWatcherBase::finished, this, &DriveAssistantDialog::testFinished);
    connect(ui.groupBox_details, &QGroupBox::toggled, ui.plainTextEdit_details, &QWidget::setVisible);

    ui.label_drive->setText(i18n("Drive: %1", drive.displayName.isEmpty() ? drive.id : drive.displayName));
    ui.treeWidget_checks->setHeaderLabels({i18n("Check"), i18n("Result")});
    ui.treeWidget_checks->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui.progressBar->setRange(0, 1);
    ui.progressBar->setValue(0);
}

DriveAssistantDialog::~DriveAssistantDialog()
{
    if (m_cancel)
        *m_cancel = true;
    m_watcher.waitForFinished();
}

Audex::Rip::DriveFeatures DriveAssistantDialog::features() const
{
    return m_result.features;
}

bool DriveAssistantDialog::offsetFound() const
{
    return m_result.offsetFound;
}

int DriveAssistantDialog::offset() const
{
    return m_result.offset;
}

void DriveAssistantDialog::startOrStop()
{
    if (m_watcher.isRunning()) {
        if (m_cancel)
            *m_cancel = true;
        m_startButton->setEnabled(false);
        ui.label_status->setText(i18n("Stopping the test..."));
        return;
    }

    ui.treeWidget_checks->clear();
    ui.plainTextEdit_details->clear();
    m_result = Result();
    m_steps = ui.checkBox_offset->isChecked() ? 8 : 7;
    setRunning(true);

    const Audex::DriveEntry drive = m_drive;
    const bool withOffset = ui.checkBox_offset->isChecked();
    m_cancel = std::make_shared<std::atomic_bool>(false);
    const std::shared_ptr<std::atomic_bool> cancel = m_cancel;

    // The checks run in a worker thread; the callbacks hop back into the GUI
    // thread. The dialog waits for the worker in its destructor.
    Audex::Rip::FeatureCallbacks cb;
    cb.started = [this](int index, int total, const Audex::Rip::FeatureCheck &check) {
        QMetaObject::invokeMethod(
            this,
            [this, index, total, check] {
                checkStarted(index, total, check);
            },
            Qt::QueuedConnection);
    };
    cb.finished = [this](const Audex::Rip::FeatureCheck &check) {
        QMetaObject::invokeMethod(
            this,
            [this, check] {
                checkFinished(check);
            },
            Qt::QueuedConnection);
    };
    cb.isCanceled = [cancel] {
        return cancel->load();
    };

    m_watcher.setFuture(QtConcurrent::run(Audex::driveThreadPool(), [drive, withOffset, cb] {
        Result result;
        const Audex::DiscReadResult disc = Audex::readDisc(drive);
        if (!disc.ok) {
            result.error = disc.error;
            return result;
        }
        if (disc.info.toc().audioTrackNumbers().isEmpty()) {
            result.error = u"The disc in the drive has no audio tracks."_s;
            return result;
        }
        Audex::OpenedReader opened = Audex::openReader(drive);
        if (!opened.reader) {
            result.error = opened.error;
            return result;
        }

        // the read offset is the only check that needs the AccurateRip database
        Audex::Rip::FeatureCallbacks callbacks = cb;
        if (withOffset) {
            callbacks.offset = [&] {
                Audex::Rip::FeatureCheck check;
                QString error;
                const QList<Audex::AccurateRip::Response> responses =
                    Audex::AccurateRip::lookupDiscEntry(disc.info.toc(), &error, nullptr, 30000, cb.isCanceled);
                if (responses.isEmpty()) {
                    check.summary = error.isEmpty() ? u"not measured: this disc is not in the AccurateRip database"_s : u"not measured: %1"_s.arg(error);
                    return check;
                }
                const Audex::AccurateRip::OffsetDetection detection =
                    Audex::AccurateRip::detectReadOffset(*opened.reader, disc.info.toc(), responses, cb.isCanceled);
                result.offsetFound = detection.found;
                result.offset = detection.offset;
                check.verdict = detection.found ? Audex::Rip::Feature::Yes : Audex::Rip::Feature::Unknown;
                check.summary = detection.found ? u"%1 samples"_s.arg(detection.offset) : u"not measured"_s;
                if (!detection.details.isEmpty())
                    check.details << detection.details;
                return check;
            };
        }
        result.features = Audex::Rip::detectDriveFeatures(*opened.reader, disc.info.toc(), callbacks);

        // a drive that stopped answering would let every further command
        // (unlocking the tray, resetting the speed) wait for its timeout;
        // Audex leaves it alone until it is switched off and on
        if (result.features.driveStalled)
            Audex::markDriveStalled(drive.id);
        else
            opened.release();
        return result;
    }));
}

void DriveAssistantDialog::checkStarted(int index, int total, const Audex::Rip::FeatureCheck &check)
{
    ui.progressBar->setRange(0, std::max(total, m_steps));
    ui.progressBar->setValue(index);
    ui.label_status->setText(i18n("Measuring: %1...", checkLabel(check)));

    auto *item = new QTreeWidgetItem(ui.treeWidget_checks);
    item->setText(0, checkLabel(check));
    item->setText(1, i18n("running..."));
}

void DriveAssistantDialog::checkFinished(const Audex::Rip::FeatureCheck &check)
{
    QTreeWidgetItem *item = ui.treeWidget_checks->topLevelItem(ui.treeWidget_checks->topLevelItemCount() - 1);
    if (!item)
        return;
    item->setText(0, checkLabel(check));
    item->setText(1, check.summary);
    item->setIcon(0, verdictIcon(check.verdict));

    ui.plainTextEdit_details->appendPlainText(u"%1: %2"_s.arg(check.name, check.summary));
    for (const QString &line : check.details)
        ui.plainTextEdit_details->appendPlainText(u"    "_s + line);
    ui.progressBar->setValue(ui.progressBar->value() + 1);
    // All checks are done, but the drive is released only now; a drive that
    // timed out before can take a while to answer that.
    if (ui.progressBar->value() >= ui.progressBar->maximum() && !(m_cancel && *m_cancel))
        ui.label_status->setText(i18n("Finishing the test and releasing the drive..."));
}

void DriveAssistantDialog::setRunning(bool running)
{
    m_startButton->setEnabled(true);
    m_startButton->setText(running ? i18n("Stop") : i18n("Start Test"));
    ui.busyIndicator->setVisible(running);
    ui.busyIndicator->setRunning(running);
    m_applyButton->setEnabled(!running && m_result.features.measured.isValid());
    ui.checkBox_offset->setEnabled(!running);
    if (running)
        ui.label_status->setText(i18n("Measuring..."));
}

void DriveAssistantDialog::testFinished()
{
    m_result = m_watcher.result();
    setRunning(false);

    if (!m_result.error.isEmpty()) {
        ui.progressBar->setValue(0);
        ui.label_status->setText(i18n("The drive could not be tested: %1", m_result.error));
        return;
    }
    if (m_result.features.canceled) {
        ui.label_status->setText(i18n("Stopped. Nothing has been changed."));
        return;
    }
    ui.progressBar->setValue(ui.progressBar->maximum());
    if (m_result.features.driveStalled) {
        Audex::Rip::FeatureCheck culprit;
        culprit.id = m_result.features.stalledAfter;
        ui.label_status->setText(culprit.id.isEmpty()
                                     ? i18n("The drive stopped answering after a command timed out. Switch it off and on again (a USB drive: unplug it) "
                                            "before using it. Apply stores what was measured.")
                                     : i18n("The drive stopped answering during the check \"%1\". Switch it off and on again (a USB drive: unplug it) "
                                            "before using it. Apply stores what was measured.",
                                            checkLabel(culprit)));
        return;
    }
    ui.label_status->setText(
        m_result.offsetFound
            ? i18n("Done. Apply stores the results for this drive, fills in the settings that follow from them and enters the read offset %1.", m_result.offset)
            : i18n("Done. Apply stores the results for this drive and fills in the settings that follow from them."));
}
