/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatadialog.h"

#include "dialogs/errordialog.h"
#include "dialogs/filenameschemewizarddialog.h"
#include "dialogs/profiledatacoverdialog.h"
#include "dialogs/profiledatahookdialog.h"
#include "dialogs/profiledatalogfiledialog.h"
#include "dialogs/profiledataplaylistdialog.h"
#include "dialogs/schemewizarddialog.h"

#include <KMessageWidget>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QVBoxLayout>

ProfileDataDialog::ProfileDataDialog(ProfileModel *profileModel, const int profileRow, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    profile_model = profileModel;
    if (!profile_model) {
        qWarning() << "ProfileDataDialog() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    if (profileRow < 0) { // find next free row index
        int row = 0;
        while (profile_model->hasIndex(row, PROFILE_MODEL_COLUMN_NAME_INDEX))
            ++row;
        profile_model->insertRows(row, 1);
        profile_row = row;
        new_profile_mode = true;
    } else {
        profile_row = profileRow;
        new_profile_mode = false;
    }

    applyButton = nullptr;

    QWidget *widget = new QWidget(this);
    ui.setupUi(widget);

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(widget);

    // For a new profile the widgets start from their own defaults, so the
    // stored parameters are only read when an existing profile is modified.
    auto storedParameters = [this](const int column) {
        Parameters parameters;
        if (!new_profile_mode)
            parameters.fromString(profile_model->data(profile_model->index(profile_row, column)).toString());
        return parameters;
    };

    lame_widget = new lameWidget(storedParameters(PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_INDEX), this);
    connect(lame_widget, &lameWidget::triggerChanged, this, &ProfileDataDialog::trigger_changed);
    opusenc_widget = new opusencWidget(storedParameters(PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_INDEX), this);
    connect(opusenc_widget, &opusencWidget::triggerChanged, this, &ProfileDataDialog::trigger_changed);
    flac_widget = new flacWidget(storedParameters(PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX), this);
    connect(flac_widget, &flacWidget::triggerChanged, this, &ProfileDataDialog::trigger_changed);
    wave_widget = new waveWidget(storedParameters(PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX), this);
    connect(wave_widget, &waveWidget::triggerChanged, this, &ProfileDataDialog::trigger_changed);
    custom_widget = new customWidget(storedParameters(PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX), this);
    connect(custom_widget, &customWidget::triggerChanged, this, &ProfileDataDialog::trigger_changed);

    ui.stackedWidget_encoder->addWidget(lame_widget);
    ui.stackedWidget_encoder->addWidget(opusenc_widget);
    ui.stackedWidget_encoder->addWidget(flac_widget);
    ui.stackedWidget_encoder->addWidget(wave_widget);
    ui.stackedWidget_encoder->addWidget(custom_widget);

    // All encoders are listed: one without its plugin stays visible (the
    // profile may use it), but cannot be chosen (see update_encoder_items()).
    for (int e = 0; e < EncoderAssistant::NUM; ++e)
        ui.kcombobox_encoder->addItem(EncoderAssistant::name((EncoderAssistant::Encoder)e), e);
    connect(ui.kcombobox_encoder, &QComboBox::activated, this, &ProfileDataDialog::set_encoder_by_combobox);

    ui.kcombobox_output->addItem(i18n("One file per track"), PROFILE_OUTPUT_TRACKS);
    ui.kcombobox_output->addItem(i18n("Whole disc as one image file"), PROFILE_OUTPUT_IMAGE);
    connect(ui.kcombobox_output, &QComboBox::activated, this, &ProfileDataDialog::set_output_by_combobox);

    encoder_message = new KMessageWidget(widget);
    encoder_message->setMessageType(KMessageWidget::Warning);
    encoder_message->setCloseButtonVisible(false);
    encoder_message->setWordWrap(true);
    encoder_message->hide();
    ui.verticalLayout->insertWidget(ui.verticalLayout->indexOf(ui.stackedWidget_encoder), encoder_message);

    connect(ui.kpushbutton_scheme, &QAbstractButton::clicked, this, &ProfileDataDialog::scheme_wizard);
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));
    connect(ui.kpushbutton_image_scheme, &QAbstractButton::clicked, this, &ProfileDataDialog::image_scheme_wizard);
    ui.kpushbutton_image_scheme->setIcon(QIcon::fromTheme("tools-wizard"));
    connect(ui.kpushbutton_cue_scheme, &QAbstractButton::clicked, this, &ProfileDataDialog::cue_scheme_wizard);
    ui.kpushbutton_cue_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    connect(ui.kpushbutton_cover, &QAbstractButton::clicked, this, &ProfileDataDialog::cover_settings);
    connect(ui.kpushbutton_playlist, &QAbstractButton::clicked, this, &ProfileDataDialog::playlist_settings);
    connect(ui.kpushbutton_logfile, &QAbstractButton::clicked, this, &ProfileDataDialog::logfile_settings);
    connect(ui.kpushbutton_hook, &QAbstractButton::clicked, this, &ProfileDataDialog::hook_settings);

    connect(ui.checkBox_cover, &QAbstractButton::toggled, this, &ProfileDataDialog::enable_settings_cover);
    connect(ui.checkBox_playlist, &QAbstractButton::toggled, this, &ProfileDataDialog::enable_settings_playlist);
    connect(ui.checkBox_logfile, &QAbstractButton::toggled, this, &ProfileDataDialog::enable_settings_logfile);
    connect(ui.checkBox_hook, &QAbstractButton::toggled, this, &ProfileDataDialog::enable_settings_hook);
    connect(ui.checkBox_cue, &QAbstractButton::toggled, this, &ProfileDataDialog::enable_settings_cue);
    connect(ui.checkBox_ctdb_repair, &QAbstractButton::toggled, this, &ProfileDataDialog::enable_settings_ctdb_repair);

    connect(this, &QDialog::rejected, this, &ProfileDataDialog::slotRejected);

    if (!new_profile_mode) {
        setWindowTitle(i18n("Modify Profile"));

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
        QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        applyButton = buttonBox->button(QDialogButtonBox::Apply);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataDialog::slotAccepted);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataDialog::reject);
        connect(applyButton, &QPushButton::clicked, this, &ProfileDataDialog::slotApplied);
        mainLayout->addWidget(buttonBox);

        ui.qlineedit_name->setText(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_NAME_INDEX)).toString());
        connect(ui.qlineedit_name, &QLineEdit::textEdited, this, &ProfileDataDialog::trigger_changed);
        ui.qlineedit_name->setCursorPosition(0);

        ui.kiconbutton_icon->setIcon(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ICON_INDEX)).toString());
        connect(ui.kiconbutton_icon, &KIconButton::iconChanged, this, &ProfileDataDialog::trigger_changed);

        set_encoder(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX)).toInt());
        connect(ui.kcombobox_encoder, &QComboBox::activated, this, &ProfileDataDialog::trigger_changed);

        set_output(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_OUTPUT_INDEX)).toInt());
        connect(ui.kcombobox_output, &QComboBox::activated, this, &ProfileDataDialog::trigger_changed);

        ui.qlineedit_image_scheme->setText(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_IMAGE_SCHEME_INDEX)).toString());
        connect(ui.qlineedit_image_scheme, &QLineEdit::textEdited, this, &ProfileDataDialog::trigger_changed);
        ui.qlineedit_image_scheme->setCursorPosition(0);

        ui.checkBox_cue->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_INDEX)).toBool());
        enable_settings_cue(ui.checkBox_cue->isChecked());
        connect(ui.checkBox_cue, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        ui.qlineedit_cue_scheme->setText(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX)).toString());
        connect(ui.qlineedit_cue_scheme, &QLineEdit::textEdited, this, &ProfileDataDialog::trigger_changed);
        ui.qlineedit_cue_scheme->setCursorPosition(0);

        ui.checkBox_cue_mcn_isrc->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX)).toBool());
        connect(ui.checkBox_cue_mcn_isrc, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        ui.checkBox_ctdb_repair->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CTDB_REPAIR_INDEX)).toBool());
        enable_settings_ctdb_repair(ui.checkBox_ctdb_repair->isChecked());
        connect(ui.checkBox_ctdb_repair, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        ui.checkBox_ctdb_repair_keep_original->setChecked(
            profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CTDB_REPAIR_KEEP_ORIGINAL_INDEX)).toBool());
        connect(ui.checkBox_ctdb_repair_keep_original, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        ui.qlineedit_scheme->setText(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SCHEME_INDEX)).toString());
        connect(ui.qlineedit_scheme, &QLineEdit::textEdited, this, &ProfileDataDialog::trigger_changed);
        ui.qlineedit_scheme->setCursorPosition(0);

        ui.checkBox_fat32compatible->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool());
        connect(ui.checkBox_fat32compatible, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        ui.checkBox_underscore->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool());
        connect(ui.checkBox_underscore, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        ui.checkBox_2digitstracknum->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX)).toBool());
        connect(ui.checkBox_2digitstracknum, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        ui.checkBox_cover->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_INDEX)).toBool());
        enable_settings_cover(ui.checkBox_cover->isChecked());
        connect(ui.checkBox_cover, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        ui.checkBox_playlist->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_INDEX)).toBool());
        enable_settings_playlist(ui.checkBox_playlist->isChecked());
        connect(ui.checkBox_playlist, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        ui.checkBox_logfile->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_INDEX)).toBool());
        enable_settings_logfile(ui.checkBox_logfile->isChecked());
        connect(ui.checkBox_logfile, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        ui.checkBox_hook->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HOOK_INDEX)).toBool());
        enable_settings_hook(ui.checkBox_hook->isChecked());
        connect(ui.checkBox_hook, &QAbstractButton::toggled, this, &ProfileDataDialog::trigger_changed);

        applyButton->setEnabled(false);

    } else {
        setWindowTitle(i18n("Create Profile"));

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataDialog::slotAccepted);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataDialog::reject);

        mainLayout->addWidget(buttonBox);

        ui.qlineedit_name->setText(i18n("New Profile"));
        ui.kiconbutton_icon->setIcon(DEFAULT_ICON);

        int encoder = DEFAULT_ENCODER_SELECTED;
        if (!EncoderAssistant::available((EncoderAssistant::Encoder)encoder))
            encoder = EncoderAssistant::available(EncoderAssistant::FLAC) ? EncoderAssistant::FLAC : EncoderAssistant::WAVE;
        set_encoder(encoder);
        set_output(DEFAULT_OUTPUT);

        ui.qlineedit_image_scheme->setText(DEFAULT_IMAGE_SCHEME);
        ui.checkBox_cue->setChecked(DEFAULT_CUE);
        ui.qlineedit_cue_scheme->setText(DEFAULT_CUE_NAME);
        ui.checkBox_cue_mcn_isrc->setChecked(DEFAULT_CUE_MCN_ISRC);
        ui.checkBox_ctdb_repair->setChecked(DEFAULT_CTDB_REPAIR);
        ui.checkBox_ctdb_repair_keep_original->setChecked(DEFAULT_CTDB_REPAIR_KEEP_ORIGINAL);
        enable_settings_cue(ui.checkBox_cue->isChecked());
        enable_settings_ctdb_repair(ui.checkBox_ctdb_repair->isChecked());

        ui.qlineedit_scheme->setText(DEFAULT_SCHEME);
        ui.checkBox_fat32compatible->setChecked(DEFAULT_FAT32);
        ui.checkBox_underscore->setChecked(DEFAULT_UNDERSCORE);
        ui.checkBox_2digitstracknum->setChecked(DEFAULT_2DIGITSTRACKNUM);

        ui.checkBox_cover->setChecked(DEFAULT_SC);
        ui.checkBox_playlist->setChecked(DEFAULT_PL);
        ui.checkBox_logfile->setChecked(DEFAULT_LOG);
        ui.checkBox_hook->setChecked(DEFAULT_HOOK);

        enable_settings_cover(ui.checkBox_cover->isChecked());
        enable_settings_playlist(ui.checkBox_playlist->isChecked());
        enable_settings_logfile(ui.checkBox_logfile->isChecked());
        enable_settings_hook(ui.checkBox_hook->isChecked());
    }

    ui.qlineedit_name->setFocus();
    resize(0, 0); // For some reason dialog start of big...
}

void ProfileDataDialog::slotAccepted()
{
    if (save())
        accept();
    else
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataDialog::slotApplied()
{
    if (!save())
        ErrorDialog::show(this, error.message(), error.details());
}

void ProfileDataDialog::slotRejected()
{
    if (!profile_model) {
        qWarning() << "ProfileDataDialog::slotRejected() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    if (new_profile_mode)
        profile_model->removeRows(profile_row, 1);
}

void ProfileDataDialog::set_encoder(const int encoder)
{
    set_encoder_widget((EncoderAssistant::Encoder)encoder);

    ui.kcombobox_encoder->setCurrentIndex(ui.kcombobox_encoder->findData(encoder));
    update_encoder_message();
}

void ProfileDataDialog::set_encoder_by_combobox(const int index)
{
    set_encoder_widget((EncoderAssistant::Encoder)ui.kcombobox_encoder->itemData(index).toInt());
    update_encoder_message();
}

void ProfileDataDialog::set_output_by_combobox(const int index)
{
    set_output(ui.kcombobox_output->itemData(index).toInt());
}

EncoderAssistant::Encoder ProfileDataDialog::selected_encoder() const
{
    if (ui.kcombobox_encoder->currentIndex() < 0)
        return EncoderAssistant::NUM;
    return (EncoderAssistant::Encoder)ui.kcombobox_encoder->currentData().toInt();
}

bool ProfileDataDialog::image_output() const
{
    return ui.kcombobox_output->currentData().toInt() == PROFILE_OUTPUT_IMAGE;
}

void ProfileDataDialog::set_output(const int output)
{
    ui.kcombobox_output->setCurrentIndex(ui.kcombobox_output->findData(output));
    const bool image = image_output();

    update_encoder_items();
    if (image && !EncoderAssistant::lossless(selected_encoder()))
        set_encoder(EncoderAssistant::available(EncoderAssistant::FLAC) ? EncoderAssistant::FLAC : EncoderAssistant::WAVE);

    // tracks: Filenames tab and playlist - image: Image tab
    const int filenamesTab = ui.tabWidget->indexOf(ui.tab_3);
    const int imageTab = ui.tabWidget->indexOf(ui.tab_image);
    ui.tabWidget->setTabEnabled(filenamesTab, !image);
    ui.tabWidget->setTabToolTip(filenamesTab, image ? i18n("Only used when ripping one file per track") : QString());
    ui.tabWidget->setTabEnabled(imageTab, image);
    ui.tabWidget->setTabToolTip(imageTab, image ? QString() : i18n("Only used when ripping the whole disc as one image file"));
    ui.checkBox_playlist->setEnabled(!image);
    enable_settings_playlist(ui.checkBox_playlist->isChecked());
}

void ProfileDataDialog::update_encoder_items()
{
    auto *model = qobject_cast<QStandardItemModel *>(ui.kcombobox_encoder->model());
    if (!model)
        return;

    const bool image = image_output();
    for (int i = 0; i < ui.kcombobox_encoder->count(); ++i) {
        const auto encoder = (EncoderAssistant::Encoder)ui.kcombobox_encoder->itemData(i).toInt();
        const bool available = EncoderAssistant::available(encoder);
        const bool allowed = !image || EncoderAssistant::lossless(encoder);
        QStandardItem *item = model->item(i);
        item->setText(available ? EncoderAssistant::name(encoder) : i18nc("@item encoder", "%1 (not installed)", EncoderAssistant::name(encoder)));
        item->setToolTip(!available ? EncoderAssistant::unavailableReason(encoder) : !allowed ? i18n("An image requires a lossless encoder.") : QString());
        item->setEnabled(available && allowed);
    }
}

void ProfileDataDialog::update_encoder_message()
{
    const QString reason = (selected_encoder() != EncoderAssistant::NUM) ? EncoderAssistant::unavailableReason(selected_encoder()) : QString();
    if (reason.isEmpty()) {
        encoder_message->hide();
        return;
    }
    encoder_message->setText(i18n("%1 The settings are kept, but the profile cannot be used until the plugin is installed.", reason));
    encoder_message->show();
}

void ProfileDataDialog::trigger_changed()
{
    if (!profile_model) {
        qWarning() << "ProfileDataDialog::trigger_changed() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    if (applyButton) {
        applyButton->setEnabled(
            ui.qlineedit_name->text() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_NAME_INDEX)).toString()
            || ui.kiconbutton_icon->icon() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ICON_INDEX)).toString()
            || ui.kcombobox_encoder->itemData(ui.kcombobox_encoder->currentIndex())
                != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX)).toString()
            || ui.qlineedit_scheme->text() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SCHEME_INDEX)).toString()
            || ui.checkBox_fat32compatible->isChecked()
                != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool()
            || ui.checkBox_underscore->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool()
            || ui.checkBox_cover->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_INDEX)).toBool()
            || ui.checkBox_playlist->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_INDEX)).toBool()
            || ui.checkBox_logfile->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_INDEX)).toBool()
            || ui.checkBox_hook->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HOOK_INDEX)).toBool()
            || ui.checkBox_2digitstracknum->isChecked()
                != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX)).toBool()
            || ui.kcombobox_output->currentData().toInt() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_OUTPUT_INDEX)).toInt()
            || ui.qlineedit_image_scheme->text() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_IMAGE_SCHEME_INDEX)).toString()
            || ui.checkBox_cue->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_INDEX)).toBool()
            || ui.qlineedit_cue_scheme->text() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX)).toString()
            || ui.checkBox_cue_mcn_isrc->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX)).toBool()
            || ui.checkBox_ctdb_repair->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CTDB_REPAIR_INDEX)).toBool()
            || ui.checkBox_ctdb_repair_keep_original->isChecked()
                != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CTDB_REPAIR_KEEP_ORIGINAL_INDEX)).toBool()
            || lame_widget->isChanged() || opusenc_widget->isChanged() || flac_widget->isChanged() || wave_widget->isChanged() || custom_widget->isChanged());
    }
}

void ProfileDataDialog::enable_settings_cover(bool enabled)
{
    ui.kpushbutton_cover->setEnabled(enabled);
}

void ProfileDataDialog::enable_settings_playlist(bool enabled)
{
    ui.kpushbutton_playlist->setEnabled(enabled && !image_output());
}

void ProfileDataDialog::enable_settings_logfile(bool enabled)
{
    ui.kpushbutton_logfile->setEnabled(enabled);
}

void ProfileDataDialog::enable_settings_hook(bool enabled)
{
    ui.kpushbutton_hook->setEnabled(enabled);
}

void ProfileDataDialog::enable_settings_cue(bool enabled)
{
    ui.label_cue_scheme->setEnabled(enabled);
    ui.qlineedit_cue_scheme->setEnabled(enabled);
    ui.kpushbutton_cue_scheme->setEnabled(enabled);
    ui.checkBox_cue_mcn_isrc->setEnabled(enabled);
}

void ProfileDataDialog::enable_settings_ctdb_repair(bool enabled)
{
    ui.checkBox_ctdb_repair_keep_original->setEnabled(enabled);
}

void ProfileDataDialog::scheme_wizard()
{
    SchemeWizardDialog dialog(ui.qlineedit_scheme->text(), this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    ui.qlineedit_scheme->setText(dialog.scheme);
    trigger_changed();
}

void ProfileDataDialog::image_scheme_wizard()
{
    FilenameSchemeWizardDialog dialog(ui.qlineedit_image_scheme->text(),
                                      selected_encoder() == EncoderAssistant::FLAC ? QStringLiteral("flac") : QStringLiteral("wav"),
                                      this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    ui.qlineedit_image_scheme->setText(dialog.scheme);
    trigger_changed();
}

void ProfileDataDialog::cue_scheme_wizard()
{
    FilenameSchemeWizardDialog dialog(ui.qlineedit_cue_scheme->text(), QStringLiteral("cue"), this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    ui.qlineedit_cue_scheme->setText(dialog.scheme);
    trigger_changed();
}

void ProfileDataDialog::cover_settings()
{
    ProfileDataCoverDialog dialog(profile_model, profile_row, new_profile_mode, this);
    if (dialog.exec() == QDialog::Accepted)
        trigger_changed();
}

void ProfileDataDialog::playlist_settings()
{
    ProfileDataPlaylistDialog dialog(profile_model, profile_row, new_profile_mode, this);
    if (dialog.exec() == QDialog::Accepted)
        trigger_changed();
}

void ProfileDataDialog::logfile_settings()
{
    ProfileDataLogFileDialog dialog(profile_model, profile_row, new_profile_mode, this);
    if (dialog.exec() == QDialog::Accepted)
        trigger_changed();
}

void ProfileDataDialog::hook_settings()
{
    ProfileDataHookDialog dialog(profile_model, profile_row, new_profile_mode, this);
    if (dialog.exec() == QDialog::Accepted)
        trigger_changed();
}

void ProfileDataDialog::set_encoder_widget(const EncoderAssistant::Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        ui.stackedWidget_encoder->setCurrentWidget(lame_widget);
        break;
    case EncoderAssistant::OPUSENC:
        ui.stackedWidget_encoder->setCurrentWidget(opusenc_widget);
        break;
    case EncoderAssistant::FLAC:
        ui.stackedWidget_encoder->setCurrentWidget(flac_widget);
        break;
    case EncoderAssistant::WAVE:
        ui.stackedWidget_encoder->setCurrentWidget(wave_widget);
        break;
    case EncoderAssistant::CUSTOM:
        ui.stackedWidget_encoder->setCurrentWidget(custom_widget);
        break;
    case EncoderAssistant::NUM:
        break;
    }
}

bool ProfileDataDialog::save()
{
    if (!profile_model) {
        qWarning() << "ProfileDataDialog::save() called with null model pointers";
        Q_ASSERT(profile_model);
        return false;
    }

    bool success = true;

    error.clear();

    if (success)
        success = lame_widget->save();
    if (!success)
        error = lame_widget->lastError();

    if (success) {
        success = opusenc_widget->save();
        if (!success)
            error = opusenc_widget->lastError();
    }

    if (success) {
        success = flac_widget->save();
        if (!success)
            error = flac_widget->lastError();
    }

    if (success) {
        success = wave_widget->save();
        if (!success)
            error = wave_widget->lastError();
    }

    if (success) {
        success = custom_widget->save();
        if (!success)
            error = custom_widget->lastError();
    }

    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_NAME_INDEX), ui.qlineedit_name->text());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ICON_INDEX), ui.kiconbutton_icon->icon());
    if (success && ui.kcombobox_encoder->currentIndex() >= 0)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX),
                                         ui.kcombobox_encoder->itemData(ui.kcombobox_encoder->currentIndex()));
    // after the encoder: the model checks the output type against it
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_OUTPUT_INDEX), ui.kcombobox_output->currentData().toInt());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SCHEME_INDEX), ui.qlineedit_scheme->text());
    if (success)
        success =
            profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX), ui.checkBox_fat32compatible->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX), ui.checkBox_underscore->isChecked());
    if (success)
        success =
            profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX), ui.checkBox_2digitstracknum->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_INDEX), ui.checkBox_cover->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_INDEX), ui.checkBox_playlist->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_INDEX), ui.checkBox_logfile->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HOOK_INDEX), ui.checkBox_hook->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_IMAGE_SCHEME_INDEX), ui.qlineedit_image_scheme->text());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_INDEX), ui.checkBox_cue->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX), ui.qlineedit_cue_scheme->text());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX), ui.checkBox_cue_mcn_isrc->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CTDB_REPAIR_INDEX), ui.checkBox_ctdb_repair->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CTDB_REPAIR_KEEP_ORIGINAL_INDEX),
                                         ui.checkBox_ctdb_repair_keep_original->isChecked());
    if (success)
        success =
            profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_INDEX), lame_widget->parameters().toString());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_INDEX),
                                         opusenc_widget->parameters().toString());
    if (success)
        success =
            profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX), flac_widget->parameters().toString());
    if (success)
        success =
            profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX), wave_widget->parameters().toString());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX),
                                         custom_widget->parameters().toString());

    if (!success)
        error = profile_model->lastError();

    if (success) {
        profile_model->commit();
        if (applyButton)
            applyButton->setEnabled(false);
        return true;
    }

    return false;
}
