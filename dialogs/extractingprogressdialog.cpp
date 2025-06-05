/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "extractingprogressdialog.h"

namespace Audex
{

ExtractingProgressDialog::ExtractingProgressDialog(QPointer<ProfileModel> profile_model,
                                                   const Audex::CDDA &cdda,
                                                   const Audex::TracknumberSet &selectedTracks,
                                                   const QByteArray &blockDevice,
                                                   QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("Rip And Encode"));

    mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &ExtractingProgressDialog::slotCancel);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    this->profile_model = profile_model;
    this->cdda = cdda;
    this->selected_tracks = selectedTracks;

    QString title = QString("%1 - %2").arg(cdda.metadata().get(Audex::Metadata::Artist).toString(), cdda.metadata().get(Audex::Metadata::Title).toString());
    ui.label_header->setText(title);

    p_single_file = profile_model->data(profile_model->index(profile_model->currentProfileRow(), PROFILE_MODEL_COLUMN_SF_INDEX)).toBool();

    if (p_single_file) {
        ui.label_extracting->setText(i18n("Ripping whole CD as single track"));
        ui.label_encoding->setText(i18n("Encoding"));

    } else {
        ui.label_extracting->setText(i18n("Ripping Track 0 of %1", cdda.toc().audioTrackCount()));
        ui.label_encoding->setText(i18n("Encoding Track 0 of %1", cdda.toc().audioTrackCount()));
    }

    audex_rip_manager = new Audex::AudexRipManager(this, profile_model, cdda, blockDevice);

    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::error, this, &ExtractingProgressDialog::show_error);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::warning, this, &ExtractingProgressDialog::show_warning);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::info, this, &ExtractingProgressDialog::show_info);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::finished, this, &ExtractingProgressDialog::conclusion);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::speedEncode, this, &ExtractingProgressDialog::show_speed_encode);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::speedExtract, this, &ExtractingProgressDialog::show_speed_extract);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::progressExtractTrack, this, &ExtractingProgressDialog::show_progress_extract_track);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::progressExtractOverall, this, &ExtractingProgressDialog::show_progress_extract_overall);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::progressEncodeTrack, this, &ExtractingProgressDialog::show_progress_encode_track);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::progressEncodeOverall, this, &ExtractingProgressDialog::show_progress_encode_overall);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::changedExtractTrack, this, &ExtractingProgressDialog::show_changed_extract_track);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::changedEncodeTrack, this, &ExtractingProgressDialog::show_changed_encode_track);
    QObject::connect(audex_rip_manager, &Audex::AudexRipManager::timeout, this, &ExtractingProgressDialog::ask_timeout);
    QObject::connect(ui.details_button, &QPushButton::pressed, this, &ExtractingProgressDialog::toggle_details);

    finished = false;

    progressbar_np_flag = false;

    unity_message = QDBusMessage::createSignal("/Audex", "com.canonical.Unity.LauncherEntry", "Update");
}

int ExtractingProgressDialog::exec()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    KConfigGroup grp(KSharedConfig::openConfig(), "ExtractingProgressDialog");

    resize(600, 400);
    current_extract_overall = 0;
    current_encode_overall = 0;
    ui.details_button->setArrowType(grp.readEntry("Simple", true) ? Qt::UpArrow : Qt::DownArrow);
    toggle_details();
    show();
    setModal(true);
    if (audex_rip_manager->prepare(selected_tracks)) {
        audex_rip_manager->start();
    }
    int rv = QDialog::exec();

    grp.writeEntry("Simple", (Qt::DownArrow == ui.details_button->arrowType()));

    return rv;
}

void ExtractingProgressDialog::calc_overall_progress()
{
    ui.progressBar_overall->setValue((int)(((float)(current_extract_overall + current_encode_overall) / 2.0f) + .5f));
    update_unity();
}

void ExtractingProgressDialog::toggle_details()
{
    if (Qt::UpArrow == ui.details_button->arrowType()) {
        ui.details_button->setArrowType(Qt::DownArrow);
        ui.details->setVisible(false);
        ui.label_overall->setVisible(false);
        ui.label_overall_track->setVisible(true);
        ui.progressBar_overall->setVisible(true);
        resize(width(), 32);

    } else {
        ui.details_button->setArrowType(Qt::UpArrow);
        ui.details->setVisible(true);
        ui.label_overall_track->setVisible(false);

        if (selected_tracks.count() < 2) {
            ui.label_overall->setVisible(false);
            ui.progressBar_overall->setVisible(false);
        } else {
            ui.label_overall->setVisible(true);
        }
        resize(width(), 400);
    }
}

void ExtractingProgressDialog::slotCancel()
{
    cancel();
}

void ExtractingProgressDialog::slotClose()
{
    close();
}

void ExtractingProgressDialog::slotEncoderLog()
{
    open_encoder_log_view_dialog();
}

void ExtractingProgressDialog::slotExtractLog()
{
    open_extract_log_view_dialog();
}

void ExtractingProgressDialog::cancel()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    if (finished) {
        close();

    } else {
        if (KMessageBox::warningTwoActions(this,
                                           i18n("Do you really want to cancel extraction?"),
                                           i18n("Cancel"),
                                           KStandardGuiItem::cancel(),
                                           KStandardGuiItem::cont())
            == KMessageBox::PrimaryAction) {
            cancelButton->setEnabled(false);
            audex_rip_manager->cancel();
        }
    }
}

void ExtractingProgressDialog::show_changed_extract_track(int no, int total, const QString &artist, const QString &title)
{
    Q_UNUSED(artist);
    Q_UNUSED(title);

    if (!p_single_file) {
        ui.label_extracting->setText((1 == total) ? i18n("Ripping Track") : i18n("Ripping Track %1 of %2", no, total));
        ui.label_overall_track->setText((1 == total) ? i18n("Overall Progress") : i18n("Overall Progress (Ripping Track %1 of %2)", no, total));
        current_track = no;
        update_unity();

    } else {
        ui.label_extracting->setText(i18n("Ripping whole CD as single track"));
        ui.label_overall_track->setText(i18n("Overall Progress"));
    }
}

void ExtractingProgressDialog::show_changed_encode_track(int no, int total, const QString &filename)
{
    Q_UNUSED(filename);

    if (no == 0) {
        ui.label_encoding->setText("<i>" + i18n("Waiting for an encoding job...") + "</i>");
        ui.label_speed_encoding->clear();
    } else {
        if (!p_single_file)
            ui.label_encoding->setText((1 == total) ? i18n("Encoding Track") : i18n("Encoding Track %1 of %2", no, total));
    }
}

void ExtractingProgressDialog::show_progress_extract_track(int percent)
{
    ui.progressBar_extracting->setValue(percent);
}

void ExtractingProgressDialog::show_progress_extract_overall(int percent)
{
    current_extract_overall = percent;
    calc_overall_progress();
}

void ExtractingProgressDialog::show_progress_encode_track(int percent)
{
    if (percent >= 0) {
        ui.progressBar_encoding->setValue(percent);
        if (progressbar_np_flag) {
            ui.progressBar_encoding->setRange(0, 100);
            ui.progressBar_encoding->setTextVisible(true);
            progressbar_np_flag = false;
        }
    } else {
        if (!progressbar_np_flag) {
            progressbar_np_flag = true;
            ui.progressBar_encoding->setRange(0, 0);
            ui.progressBar_encoding->setTextVisible(false);
        }
    }
}

void ExtractingProgressDialog::show_progress_encode_overall(int percent)
{
    current_encode_overall = percent;
    calc_overall_progress();
}

void ExtractingProgressDialog::show_speed_encode(double speed)
{
    QString s = QString("%1").arg((double)speed, 0, 'f', 2);
    ui.label_speed_encoding->setText("<i>" + i18n("Speed: %1x", s) + "</i>");
}

void ExtractingProgressDialog::show_speed_extract(double speed)
{
    QString s = QString("%1").arg((double)speed, 0, 'f', 2);
    ui.label_speed_extracting->setText("<i>" + i18n("Speed: %1x", s) + "</i>");
}

void ExtractingProgressDialog::conclusion(bool successful)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    // Remove the cancel button
    buttonBox->clear();
    // Add the new close button
    buttonBox->addButton(QDialogButtonBox::Close);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &ExtractingProgressDialog::slotClose);

    finished = true;

    update_unity();

    QPalette pal(ui.label_extracting->palette());
    KColorScheme kcs(QPalette::Active);
    if (successful) {
        QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme("dialog-ok-apply"), i18n("All jobs successfully done."));
        ui.klistwidget->addItem(item);
        ui.klistwidget->scrollToItem(item);
        pal.setBrush(QPalette::Text, kcs.foreground(KColorScheme::PositiveText));
        ui.label_extracting->setText("<font style=\"font-weight:bold;\">" + i18n("Finished!") + "</font>");
        ui.label_encoding->setText("<font style=\"font-weight:bold;\">" + i18n("Finished!") + "</font>");
        ui.label_overall_track->setText("<font style=\"font-weight:bold;\">" + i18n("Finished!") + "</font>");
        ui.progressBar_extracting->setValue(100);
        ui.progressBar_encoding->setValue(100);
        ui.progressBar_overall->setValue(100);
    } else {
        QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme("dialog-cancel"), i18n("At least one job failed."));
        pal.setBrush(QPalette::Text, kcs.foreground(KColorScheme::NegativeText));
        ui.klistwidget->addItem(item);
        ui.klistwidget->scrollToItem(item);
        ui.label_extracting->setText("<font style=\"color:red;font-weight:bold;\">" + i18n("Failed!") + "</font>");
        ui.label_encoding->setText("<font style=\"color:red;font-weight:bold;\">" + i18n("Failed!") + "</font>");
        ui.label_overall_track->setText("<font style=\"color:red;font-weight:bold;\">" + i18n("Failed!") + "</font>");
        if (audex_rip_manager->encoderLog().count() > 0) {
            auto *encoderLogButton = new QPushButton();
            encoderLogButton->setText(i18n("Show encoding log..."));
            encoderLogButton->setIcon(QIcon::fromTheme(QStringLiteral("media-optical-audio")));
            buttonBox->addButton(encoderLogButton, QDialogButtonBox::HelpRole);
            QObject::connect(encoderLogButton, &QPushButton::clicked, this, &ExtractingProgressDialog::slotEncoderLog);
        }
        if (audex_rip_manager->extractLog().count() > 0) {
            auto *extractLogButton = new QPushButton();
            extractLogButton->setText(i18n("Show rip log..."));
            extractLogButton->setIcon(QIcon::fromTheme(QStringLiteral("media-optical")));
            buttonBox->addButton(extractLogButton, QDialogButtonBox::HelpRole);
            QObject::connect(extractLogButton, &QPushButton::clicked, this, &ExtractingProgressDialog::slotExtractLog);
        }
    }

    ui.progressBar_extracting->setEnabled(false);
    ui.progressBar_encoding->setEnabled(false);
    ui.progressBar_overall->setEnabled(false);
    ui.label_speed_extracting->setEnabled(false);
    ui.label_speed_encoding->setEnabled(false);
    ui.label_overall->setEnabled(false);

    ui.label_extracting->setPalette(pal);
    ui.label_encoding->setPalette(pal);
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

void ExtractingProgressDialog::ask_timeout()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    if (KMessageBox::questionTwoActions(this,
                                        i18n("Ripping speed was extremely slow for the last 5 minutes.\n"
                                             "Do you want to continue extraction?"),
                                        i18n("Cancel extraction"),
                                        KStandardGuiItem::cont(),
                                        KStandardGuiItem::cancel())
        == KMessageBox::SecondaryAction) {
        audex_rip_manager->cancel();
    }
}

void ExtractingProgressDialog::open_encoder_log_view_dialog()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    LogViewDialog logViewDialog(audex_rip_manager->encoderLog(), i18n("Encoding log"), this);
    logViewDialog.exec();
}

void ExtractingProgressDialog::open_extract_log_view_dialog()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    LogViewDialog logViewDialog(audex_rip_manager->extractLog(), i18n("Ripping log"), this);
    logViewDialog.exec();
}

void ExtractingProgressDialog::update_unity()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    QList<QVariant> args;
    int progress = ui.progressBar_overall->value();
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

}
