/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "devicewidget.h"

#include "dialogs/driveassistantdialog.h"
#include "utils/disccontroller.h"

#include <KMessageWidget>

#include <QCheckBox>
#include <QComboBox>
#include <QIcon>
#include <QLocale>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QWidget>
#include <QtConcurrent>

using namespace Qt::StringLiterals;

deviceWidget::deviceWidget(DiscController *discController, QWidget *parent)
    : deviceWidgetUI(parent)
    , m_discController(discController)
{
    if (!m_discController) {
        qWarning() << "deviceWidget() called with null disc controller pointers";
        Q_ASSERT(m_discController);
        return;
    }

    // inline feedback for the AccurateRip offset fetch and detection,
    // inserted between the button row and the hint label
    m_fetchMessage = new KMessageWidget(this);
    m_fetchMessage->setWordWrap(true);
    m_fetchMessage->hide();
    verticalLayout_offset->insertWidget(2, m_fetchMessage);

    // a drive without accurate stream is unsuitable: shown below the test result
    m_streamMessage = new KMessageWidget(this);
    m_streamMessage->setWordWrap(true);
    m_streamMessage->setCloseButtonVisible(false);
    m_streamMessage->setMessageType(KMessageWidget::Error);
    m_streamMessage->hide();
    verticalLayout_features->insertWidget(1, m_streamMessage);

    pushButton_fetchOffset->setIcon(QIcon::fromTheme(u"download"_s));
    pushButton_detectOffset->setIcon(QIcon::fromTheme(u"media-optical-audio"_s));
    pushButton_assistant->setIcon(QIcon::fromTheme(u"drive-optical"_s));

    fillDriveList();

    connect(comboBox_device, &QComboBox::currentIndexChanged, this, &deviceWidget::deviceSwitched);
    connect(pushButton_fetchOffset, &QPushButton::clicked, this, &deviceWidget::fetchOffset);
    connect(pushButton_detectOffset, &QPushButton::clicked, this, &deviceWidget::detectOffset);
    connect(pushButton_assistant, &QPushButton::clicked, this, &deviceWidget::runAssistant);
    connect(&m_detectWatcher, &QFutureWatcherBase::finished, this, &deviceWidget::offsetDetectionFinished);
    connect(m_discController, &DiscController::drivesChanged, this, &deviceWidget::drivesChanged);

    for (QCheckBox *box : {checkBox_secureMode, checkBox_useC2, checkBox_cacheDefeat})
        connect(box, &QCheckBox::toggled, this, &deviceWidget::updateAccuracyEnabling);
    for (QCheckBox *box : {checkBox_secureMode, checkBox_useC2, checkBox_cacheDefeat, checkBox_overread})
        connect(box, &QCheckBox::toggled, this, &deviceWidget::changed);
    for (QSpinBox *spin : {spinBox_retriesOnReadError, spinBox_sampleShift, spinBox_readSpeed, spinBox_errorReadSpeed})
        connect(spin, &QSpinBox::valueChanged, this, &deviceWidget::changed);

    m_c2ToolTip = checkBox_useC2->toolTip();
    connect(m_discController, &DiscController::discDetected, this, [this](const Audex::DiscReadResult &) {
        updateC2Availability();
    });

    loadSettings();
}

deviceWidget::~deviceWidget()
{
    // no waiting: the detection holds copies only, and a hung drive would
    // block it until the kernel gives up (see driveThreadPool())
    if (m_detectCancel)
        *m_detectCancel = true;
}

Audex::DriveEntry deviceWidget::currentDrive() const
{
    if (!m_discController) {
        qWarning() << "deviceWidget::currentDrive() called with null disc controller pointers";
        Q_ASSERT(m_discController);
        return {};
    }

    for (const DiscController::DriveInfo &d : m_discController->drives())
        if (d.udi == currentUdi())
            return d.entry;
    return {};
}

// Measures what the drive can do and fills in the settings that follow from
// the result; the values are stored when the settings dialog is closed.
void deviceWidget::runAssistant()
{
    const Audex::DriveEntry drive = currentDrive();
    if (drive.id.isEmpty())
        return;

    DriveAssistantDialog dialog(drive, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const Audex::Rip::DriveFeatures features = dialog.features();
    if (!features.measured.isValid())
        return;

    m_measured.insert(m_shownUdi, features);
    DeviceSettings::Values values = widgetValues();
    applyFeatures(features, &values);
    if (dialog.offsetFound())
        values.sampleShift = dialog.offset();
    setWidgetValues(values);
    updateFeatureSummary();
    Q_EMIT changed();
}

void deviceWidget::applyFeatures(const Audex::Rip::DriveFeatures &features, DeviceSettings::Values *values)
{
    using Audex::Rip::Feature;
    if (features.accurateStream == Feature::No)
        values->secureMode = true; // at least it shows the damage (the rip warns about the drive)
    if (features.caching != Feature::Unknown)
        values->cacheDefeat = features.caching == Feature::Yes;
    if (!features.c2Supported || features.c2Reliable == Feature::No)
        values->useC2 = false;
    else if (features.c2Reliable == Feature::Yes)
        values->useC2 = true;
    if (features.leadIn != Feature::Unknown || features.leadOut != Feature::Unknown)
        values->overread = features.leadIn == Feature::Yes || features.leadOut == Feature::Yes;
}

void deviceWidget::updateAccuracyEnabling()
{
    const bool secure = checkBox_secureMode->isChecked();
    checkBox_useC2->setEnabled(secure && !m_c2Unsupported);
    checkBox_cacheDefeat->setEnabled(secure);
}

void deviceWidget::updateFeatureSummary()
{
    using Audex::Rip::Feature;
    const Audex::Rip::DriveFeatures f = m_measured.contains(m_shownUdi) ? m_measured.value(m_shownUdi) : DeviceSettings::features(m_shownUdi);

    if (f.measured.isValid() && f.accurateStream == Feature::No) {
        m_streamMessage->setText(
            i18n("This drive does not read accurately (no accurate stream): its reads are off after every jump on the disc, and "
                 "Audex cannot compensate for this. Rips with it are not usable: secure mode reports almost every sector as "
                 "suspicious, fast mode delivers shifted audio. Please use another drive."));
        m_streamMessage->animatedShow();
    } else {
        m_streamMessage->hide();
    }

    if (!f.measured.isValid()) {
        label_features->setText(i18n("This drive has not been tested yet. The test needs an audio CD and takes about two minutes."));
        return;
    }

    QStringList parts;
    if (f.burstSectors > 0)
        parts << i18np("%1 sector per read command", "%1 sectors per read command", f.burstSectors);
    if (f.accurateStream != Feature::Unknown)
        parts << (f.accurateStream == Feature::Yes ? i18n("accurate stream") : i18n("no accurate stream, %1 samples off", f.jitterSamples));
    if (f.caching == Feature::Yes)
        parts << i18np("audio cache, %1 far read flushes it", "audio cache, %1 far reads flush it", f.cacheDefeatReads);
    else if (f.caching == Feature::No)
        parts << i18n("no audio cache");
    if (!f.c2Supported)
        parts << i18n("no C2 error pointers");
    else if (f.c2Reliable == Feature::Yes)
        parts << i18n("reliable C2 error pointers");
    else if (f.c2Reliable == Feature::No)
        parts << i18n("unreliable C2 error pointers");
    if (f.leadIn == Feature::Yes && f.leadOut == Feature::Yes)
        parts << i18n("reads lead-in and lead-out");
    else if (f.leadOut == Feature::Yes)
        parts << i18n("reads the lead-out");
    else if (f.leadIn == Feature::Yes)
        parts << i18n("reads the lead-in");
    else if (f.leadOut == Feature::No)
        parts << i18n("cannot overread");
    if (f.subchannelQ == Feature::No)
        parts << i18n("no Q sub-channel");
    if (f.rwSubchannel == Feature::No)
        parts << i18n("no CD+G graphics (raw sub-channel)");

    label_features->setText(i18n("Tested on %1: %2.", QLocale().toString(f.measured, QLocale::ShortFormat), parts.join(QStringLiteral(", "))));
}

void deviceWidget::detectOffset()
{
    if (m_detectWatcher.isRunning())
        return;
    const Audex::DriveEntry drive = currentDrive();
    if (drive.id.isEmpty())
        return;

    m_detectUdi = currentUdi();
    m_detectCancel = std::make_shared<std::atomic_bool>(false);
    m_fetchMessage->hide();
    pushButton_detectOffset->setEnabled(false);
    pushButton_detectOffset->setText(i18n("Detecting..."));

    const std::shared_ptr<std::atomic_bool> cancel = m_detectCancel;
    m_detectWatcher.setFuture(QtConcurrent::run(Audex::driveThreadPool(), [drive, cancel] {
        const auto isCanceled = [cancel] {
            return cancel->load();
        };
        Audex::AccurateRip::OffsetDetection result;
        const Audex::DiscReadResult disc = Audex::readDisc(drive);
        if (!disc.ok) {
            result.details = disc.error;
            return result;
        }
        QString error;
        const QList<Audex::AccurateRip::Response> responses = Audex::AccurateRip::lookupDiscEntry(disc.info.toc(), &error, nullptr, 30000, isCanceled);
        if (responses.isEmpty()) {
            result.details = error; // empty: not in the database
            return result;
        }
        Audex::OpenedReader opened = Audex::openReader(drive);
        if (!opened.reader) {
            result.details = opened.error;
            return result;
        }
        result = Audex::AccurateRip::detectReadOffset(*opened.reader, disc.info.toc(), responses, isCanceled);
        opened.release();
        return result;
    }));
}

void deviceWidget::offsetDetectionFinished()
{
    pushButton_detectOffset->setEnabled(true);
    pushButton_detectOffset->setText(i18n("Detect with inserted CD"));
    const Audex::AccurateRip::OffsetDetection d = m_detectWatcher.result();
    if (m_detectUdi != m_shownUdi)
        return; // another drive is on screen now
    if (d.found) {
        spinBox_sampleShift->setValue(d.offset);
        m_fetchMessage->setMessageType(KMessageWidget::Positive);
        m_fetchMessage->setText(i18n("Read offset %1 detected and entered. %2 Confirm it with a second CD: a disc from another pressing can shift the result.",
                                     d.offset,
                                     d.details));
    } else {
        m_fetchMessage->setMessageType(KMessageWidget::Warning);
        m_fetchMessage->setText(d.details.isEmpty() ? i18n("This disc is not in the AccurateRip database; try another one.")
                                                    : i18n("The read offset could not be detected: %1", d.details));
    }
    m_fetchMessage->animatedShow();
}

QString deviceWidget::currentDriveName() const
{
    if (!m_discController) {
        qWarning() << "deviceWidget::currentDriveName() called with null disc controller pointers";
        Q_ASSERT(m_discController);
        return {};
    }

    const QString udi = currentUdi();
    for (const DiscController::DriveInfo &d : m_discController->drives())
        if (d.udi == udi)
            return d.entry.displayName;
    return {};
}

void deviceWidget::fetchOffset()
{
    const QString name = currentDriveName();
    if (name.isEmpty()) {
        m_fetchMessage->setMessageType(KMessageWidget::Information);
        m_fetchMessage->setText(i18n("No drive is selected."));
        m_fetchMessage->animatedShow();
        return;
    }
    if (!m_arClient) {
        m_arClient = new Audex::AccurateRip::Client(this);
        connect(m_arClient, &Audex::AccurateRip::Client::driveOffsetsFetched, this, &deviceWidget::offsetFetchFinished);
        connect(m_arClient, &Audex::AccurateRip::Client::failed, this, &deviceWidget::offsetFetchFailed);
    }
    if (!m_arClient) {
        qWarning() << "deviceWidget::fetchOffset() could not initialize AccurateRip client";
        return;
    }
    if (m_arClient->isBusy())
        return;
    m_offsetDriveName = name;
    m_fetchMessage->hide();
    pushButton_fetchOffset->setEnabled(false);
    pushButton_fetchOffset->setText(i18n("Fetching..."));
    m_arClient->fetchDriveOffsets();
}

void deviceWidget::updateC2Availability()
{
    if (!m_discController) {
        qWarning() << "deviceWidget::updateC2Availability() called with null disc controller pointers";
        Q_ASSERT(m_discController);
        return;
    }

    const std::optional<bool> c2 = m_discController->driveSupportsC2(m_shownUdi);
    const bool unsupported = c2.has_value() && !c2.value();
    m_c2Unsupported = unsupported;
    updateAccuracyEnabling();
    checkBox_useC2->setToolTip(unsupported ? i18n("This drive does not support C2 error pointers; the setting has no effect.") : m_c2ToolTip);
    if (unsupported)
        checkBox_useC2->setChecked(false);
}

void deviceWidget::offsetFetchFinished(const QList<Audex::AccurateRip::DriveOffset> &offsets)
{
    pushButton_fetchOffset->setEnabled(true);
    pushButton_fetchOffset->setText(i18n("Fetch from AccurateRip"));

    const auto match = Audex::AccurateRip::findDriveOffset(offsets, m_offsetDriveName);
    if (!match) {
        m_fetchMessage->setMessageType(KMessageWidget::Information);
        m_fetchMessage->setText(i18n(
            "The drive \"%1\" was not found in the AccurateRip drive database. Look up a similar model name via the link below and enter the offset manually.",
            m_offsetDriveName));
        m_fetchMessage->animatedShow();
        return;
    }
    spinBox_sampleShift->setValue(match->offset);
    m_fetchMessage->setMessageType(KMessageWidget::Positive);
    m_fetchMessage->setText(
        i18n("The read offset of \"%1\" is %2 samples (AccurateRip drive database). The value has been entered.", match->name, match->offset));
    m_fetchMessage->animatedShow();
}

void deviceWidget::offsetFetchFailed(const QString &message)
{
    pushButton_fetchOffset->setEnabled(true);
    pushButton_fetchOffset->setText(i18n("Fetch from AccurateRip"));
    m_fetchMessage->setMessageType(KMessageWidget::Error);
    m_fetchMessage->setText(i18n("Could not fetch the AccurateRip drive database: %1", message));
    m_fetchMessage->animatedShow();
}

bool deviceWidget::hasChanged() const
{
    if (!m_measured.isEmpty()) // the assistant ran and its result is not stored yet
        return true;
    if (!m_shownUdi.isEmpty() && widgetValues() != DeviceSettings::load(m_shownUdi))
        return true;
    for (auto it = m_edited.cbegin(); it != m_edited.cend(); ++it)
        if (it.key() != m_shownUdi && it.value() != DeviceSettings::load(it.key()))
            return true;
    return false;
}

bool deviceWidget::isDefault() const
{
    return m_shownUdi.isEmpty() || widgetValues() == DeviceSettings::factoryDefaults();
}

void deviceWidget::loadSettings()
{
    m_edited.clear();
    m_measured.clear();
    showDrive(currentUdi());
}

void deviceWidget::saveSettings()
{
    stashShownDrive();
    for (auto it = m_edited.cbegin(); it != m_edited.cend(); ++it)
        if (it.value() != DeviceSettings::load(it.key())) // untouched drives keep following the defaults
            DeviceSettings::save(it.key(), it.value());
    for (auto it = m_measured.cbegin(); it != m_measured.cend(); ++it)
        DeviceSettings::setFeatures(it.key(), it.value());
    m_edited.clear();
    m_measured.clear();
}

void deviceWidget::setDefaults()
{
    if (!m_shownUdi.isEmpty())
        setWidgetValues(DeviceSettings::factoryDefaults());
}

void deviceWidget::deviceSwitched(int index)
{
    Q_UNUSED(index);
    stashShownDrive();
    showDrive(currentUdi());
}

void deviceWidget::drivesChanged()
{
    stashShownDrive();
    fillDriveList();
    showDrive(currentUdi());
}

void deviceWidget::fillDriveList()
{
    if (!m_discController) {
        qWarning() << "deviceWidget::fillDriveList() called with null disc controller pointers";
        Q_ASSERT(m_discController);
        return;
    }

    // keep the drive on screen, initially the active drive
    const QString selected = m_shownUdi.isEmpty() ? m_discController->currentDriveUdi() : m_shownUdi;
    const QSignalBlocker blocker(comboBox_device);
    comboBox_device->clear();
    for (const DiscController::DriveInfo &d : m_discController->drives()) {
        QString text = d.entry.displayName;
        if (!d.entry.id.isEmpty() && !text.contains(d.entry.id))
            text += u" ("_s + d.entry.id + u')';
        comboBox_device->addItem(QIcon::fromTheme(u"drive-optical"_s), text, d.udi);
    }

    const bool haveDrives = comboBox_device->count() > 0;
    if (!haveDrives)
        comboBox_device->addItem(i18n("No optical drive found"), QString());
    comboBox_device->setEnabled(haveDrives);
    label_device->setEnabled(haveDrives);
    groupBox_features->setEnabled(haveDrives);
    groupBox_accuracy->setEnabled(haveDrives);
    groupBox_offset->setEnabled(haveDrives);
    groupBox_speed->setEnabled(haveDrives);

    const int idx = comboBox_device->findData(selected);
    comboBox_device->setCurrentIndex(idx >= 0 ? idx : 0);
}

void deviceWidget::stashShownDrive()
{
    if (!m_shownUdi.isEmpty())
        m_edited[m_shownUdi] = widgetValues();
}

void deviceWidget::showDrive(const QString &udi)
{
    m_shownUdi = udi;
    m_fetchMessage->hide(); // fetch results refer to the previously shown drive
    setWidgetValues(m_edited.contains(udi) ? m_edited.value(udi) : DeviceSettings::load(udi));
    updateC2Availability();
    updateFeatureSummary();
}

QString deviceWidget::currentUdi() const
{
    return comboBox_device->itemData(comboBox_device->currentIndex()).toString();
}

DeviceSettings::Values deviceWidget::widgetValues() const
{
    DeviceSettings::Values values;
    values.secureMode = checkBox_secureMode->isChecked();
    values.useC2 = checkBox_useC2->isChecked();
    values.cacheDefeat = checkBox_cacheDefeat->isChecked();
    values.overread = checkBox_overread->isChecked();
    values.retriesOnReadError = spinBox_retriesOnReadError->value();
    values.sampleShift = spinBox_sampleShift->value();
    values.readSpeed = spinBox_readSpeed->value();
    values.errorReadSpeed = spinBox_errorReadSpeed->value();
    return values;
}

void deviceWidget::setWidgetValues(const DeviceSettings::Values &values)
{
    checkBox_secureMode->setChecked(values.secureMode);
    checkBox_useC2->setChecked(values.useC2);
    checkBox_cacheDefeat->setChecked(values.cacheDefeat);
    checkBox_overread->setChecked(values.overread);
    updateAccuracyEnabling();
    spinBox_retriesOnReadError->setValue(values.retriesOnReadError);
    spinBox_sampleShift->setValue(values.sampleShift);
    spinBox_readSpeed->setValue(values.readSpeed);
    spinBox_errorReadSpeed->setValue(values.errorReadSpeed);
}
