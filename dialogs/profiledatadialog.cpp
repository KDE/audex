/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatadialog.h"
#include "models/profilemodel.h"

#include <KConfigGroup>
#include <QDebug>
#include <QDialogButtonBox>
#include <QVBoxLayout>

ProfileDataDialog::ProfileDataDialog(ProfileModel *profileModel, const int profileRow, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    profile_model = profileModel;
    if (!profile_model) {
        qDebug() << "ProfileModel is NULL!";
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

    if (!new_profile_mode) {
        lame_parameters.fromString(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_INDEX)).toString());
        oggenc_parameters.fromString(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_OGGENC_PARAMETERS_INDEX)).toString());
        opusenc_parameters.fromString(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_INDEX)).toString());
        flac_parameters.fromString(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX)).toString());
        faac_parameters.fromString(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_FAAC_PARAMETERS_INDEX)).toString());
        wave_parameters.fromString(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX)).toString());
        custom_parameters.fromString(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX)).toString());
    }

    lame_widget = new lameWidget(&lame_parameters, this);
    connect(lame_widget, SIGNAL(triggerChanged()), this, SLOT(trigger_changed()));
    oggenc_widget = new oggencWidget(&oggenc_parameters, this);
    connect(oggenc_widget, SIGNAL(triggerChanged()), this, SLOT(trigger_changed()));
    opusenc_widget = new opusencWidget(&opusenc_parameters, this);
    connect(opusenc_widget, SIGNAL(triggerChanged()), this, SLOT(trigger_changed()));
    flac_widget = new flacWidget(&flac_parameters, this);
    connect(flac_widget, SIGNAL(triggerChanged()), this, SLOT(trigger_changed()));
    faac_widget = new faacWidget(&faac_parameters, this);
    connect(faac_widget, SIGNAL(triggerChanged()), this, SLOT(trigger_changed()));
    wave_widget = new waveWidget(&wave_parameters, this);
    connect(wave_widget, SIGNAL(triggerChanged()), this, SLOT(trigger_changed()));
    custom_widget = new customWidget(&custom_parameters, this);
    connect(custom_widget, SIGNAL(triggerChanged()), this, SLOT(trigger_changed()));

    ui.stackedWidget_encoder->addWidget(lame_widget);
    ui.stackedWidget_encoder->addWidget(oggenc_widget);
    ui.stackedWidget_encoder->addWidget(opusenc_widget);
    ui.stackedWidget_encoder->addWidget(flac_widget);
    ui.stackedWidget_encoder->addWidget(faac_widget);
    ui.stackedWidget_encoder->addWidget(wave_widget);
    ui.stackedWidget_encoder->addWidget(custom_widget);

    QMap<int, QString> encoders = EncoderAssistant::availableEncoderNameList();
    QMap<int, QString>::const_iterator i = encoders.constBegin();
    while (i != encoders.constEnd()) {
        ui.kcombobox_encoder->addItem(i.value(), i.key());
        ++i;
    }
    connect(ui.kcombobox_encoder, SIGNAL(activated(int)), this, SLOT(set_encoder_by_combobox(int)));

    connect(ui.kpushbutton_scheme, SIGNAL(clicked()), this, SLOT(scheme_wizard()));
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    connect(ui.kpushbutton_cover, SIGNAL(clicked()), this, SLOT(cover_settings()));
    connect(ui.kpushbutton_playlist, SIGNAL(clicked()), this, SLOT(playlist_settings()));
    connect(ui.kpushbutton_info, SIGNAL(clicked()), this, SLOT(info_settings()));
    connect(ui.kpushbutton_hashlist, SIGNAL(clicked()), this, SLOT(hashlist_settings()));
    connect(ui.kpushbutton_cuesheet, SIGNAL(clicked()), this, SLOT(cuesheet_settings()));
    connect(ui.kpushbutton_logfile, SIGNAL(clicked()), this, SLOT(logfile_settings()));
    connect(ui.kpushbutton_singlefile, SIGNAL(clicked()), this, SLOT(singlefile_settings()));

    connect(ui.checkBox_cover, SIGNAL(toggled(bool)), this, SLOT(enable_settings_cover(bool)));
    connect(ui.checkBox_playlist, SIGNAL(toggled(bool)), this, SLOT(enable_settings_playlist(bool)));
    connect(ui.checkBox_info, SIGNAL(toggled(bool)), this, SLOT(enable_settings_info(bool)));
    connect(ui.checkBox_hashlist, SIGNAL(toggled(bool)), this, SLOT(enable_settings_hashlist(bool)));
    connect(ui.checkBox_cuesheet, SIGNAL(toggled(bool)), this, SLOT(enable_settings_cuesheet(bool)));
    connect(ui.checkBox_logfile, SIGNAL(toggled(bool)), this, SLOT(enable_settings_logfile(bool)));
    connect(ui.checkBox_singlefile, SIGNAL(toggled(bool)), this, SLOT(enable_settings_singlefile(bool)));
    connect(ui.checkBox_singlefile, SIGNAL(toggled(bool)), this, SLOT(disable_filenames(bool)));
    connect(ui.checkBox_singlefile, SIGNAL(toggled(bool)), this, SLOT(disable_playlist(bool)));

    connect(this, SIGNAL(rejected()), this, SLOT(slotRejected()));

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
        connect(ui.qlineedit_name, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));
        ui.qlineedit_name->setCursorPosition(0);

        ui.kiconbutton_icon->setIcon(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ICON_INDEX)).toString());
        connect(ui.kiconbutton_icon, SIGNAL(iconChanged(const QString &)), this, SLOT(trigger_changed()));

        set_encoder(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX)).toInt());
        connect(ui.kcombobox_encoder, SIGNAL(activated(int)), this, SLOT(trigger_changed()));

        ui.qlineedit_scheme->setText(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SCHEME_INDEX)).toString());
        connect(ui.qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));
        ui.qlineedit_scheme->setCursorPosition(0);

        ui.checkBox_fat32compatible->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX)).toBool());
        connect(ui.checkBox_fat32compatible, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

        ui.checkBox_underscore->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX)).toBool());
        connect(ui.checkBox_underscore, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

        ui.checkBox_2digitstracknum->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX)).toBool());
        connect(ui.checkBox_2digitstracknum, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

        ui.checkBox_cover->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SC_INDEX)).toBool());
        enable_settings_cover(ui.checkBox_cover->isChecked());
        connect(ui.checkBox_cover, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

        ui.checkBox_playlist->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_INDEX)).toBool());
        enable_settings_playlist(ui.checkBox_playlist->isChecked());
        connect(ui.checkBox_playlist, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

        ui.checkBox_info->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_INF_INDEX)).toBool());
        enable_settings_info(ui.checkBox_info->isChecked());
        connect(ui.checkBox_info, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

        ui.checkBox_hashlist->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_INDEX)).toBool());
        enable_settings_hashlist(ui.checkBox_hashlist->isChecked());
        connect(ui.checkBox_hashlist, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

        ui.checkBox_cuesheet->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_INDEX)).toBool());
        enable_settings_cuesheet(ui.checkBox_cuesheet->isChecked());
        connect(ui.checkBox_cuesheet, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

        ui.checkBox_logfile->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_INDEX)).toBool());
        enable_settings_logfile(ui.checkBox_logfile->isChecked());
        connect(ui.checkBox_logfile, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

        ui.checkBox_singlefile->setChecked(profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SF_INDEX)).toBool());
        enable_settings_singlefile(ui.checkBox_singlefile->isChecked());
        connect(ui.checkBox_singlefile, SIGNAL(toggled(bool)), this, SLOT(trigger_changed()));

        if (ui.checkBox_singlefile->isChecked())
            disable_playlist(true);

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

        set_encoder(DEFAULT_ENCODER_SELECTED);

        ui.qlineedit_scheme->setText(DEFAULT_SCHEME);
        ui.checkBox_fat32compatible->setChecked(DEFAULT_FAT32);
        ui.checkBox_underscore->setChecked(DEFAULT_UNDERSCORE);
        ui.checkBox_2digitstracknum->setChecked(DEFAULT_2DIGITSTRACKNUM);

        ui.checkBox_cover->setChecked(DEFAULT_SC);
        ui.checkBox_playlist->setChecked(DEFAULT_PL);
        ui.checkBox_info->setChecked(DEFAULT_INF);
        ui.checkBox_hashlist->setChecked(DEFAULT_HL);
        ui.checkBox_cuesheet->setChecked(DEFAULT_CUE);
        ui.checkBox_logfile->setChecked(DEFAULT_LOG);
        ui.checkBox_singlefile->setChecked(DEFAULT_SF);
        cover_scale = DEFAULT_SC_SCALE;
        cover_size = DEFAULT_SC_SIZE;
        cover_format = DEFAULT_SC_FORMAT;
        cover_scheme = DEFAULT_SC_NAME;
        playlist_format = DEFAULT_PL_FORMAT;
        playlist_scheme = DEFAULT_PL_NAME;
        infofile_text.clear();
        infofile_scheme = DEFAULT_INF_NAME;
        infofile_suffix = DEFAULT_INF_SUFFIX;
        hashlist_format = DEFAULT_HL_FORMAT;
        hashlist_scheme = DEFAULT_HL_NAME;
        cuesheet_scheme = DEFAULT_CUE_NAME;
        cuesheet_write_mcn_and_isrc = DEFAULT_CUE_WRITE_MCN_AND_ISRC;
        logfile_scheme = DEFAULT_CUE_NAME;
        logfile_write_timestamps = DEFAULT_LOG_WRITE_TIMESTAMPS;
        singlefile_scheme = DEFAULT_SF_NAME;

        enable_settings_cover(ui.checkBox_cover->isChecked());
        enable_settings_playlist(ui.checkBox_playlist->isChecked());
        enable_settings_info(ui.checkBox_info->isChecked());
        enable_settings_hashlist(ui.checkBox_hashlist->isChecked());
        enable_settings_cuesheet(ui.checkBox_cuesheet->isChecked());
        enable_settings_logfile(ui.checkBox_logfile->isChecked());
        enable_settings_singlefile(ui.checkBox_singlefile->isChecked());

        disable_playlist(ui.checkBox_singlefile->isChecked());
    }

    enable_filenames(!ui.checkBox_singlefile->isChecked());

    ui.qlineedit_name->setFocus();
    resize(0, 0); // For some reason dialog start of big...
}

ProfileDataDialog::~ProfileDataDialog()
{
    delete lame_widget;
    delete oggenc_widget;
    delete opusenc_widget;
    delete flac_widget;
    delete faac_widget;
    delete wave_widget;
    delete custom_widget;
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
    if (new_profile_mode)
        profile_model->removeRows(profile_row, 1);
}

void ProfileDataDialog::set_encoder(const int encoder)
{
    set_encoder_widget((EncoderAssistant::Encoder)encoder);

    ui.kcombobox_encoder->setCurrentIndex(ui.kcombobox_encoder->findData(encoder));
}

void ProfileDataDialog::set_encoder_by_combobox(const int index)
{
    set_encoder_widget((EncoderAssistant::Encoder)ui.kcombobox_encoder->itemData(index).toInt());
}

void ProfileDataDialog::trigger_changed()
{
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
            || ui.checkBox_info->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_INF_INDEX)).toBool()
            || ui.checkBox_hashlist->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_INDEX)).toBool()
            || ui.checkBox_cuesheet->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_INDEX)).toBool()
            || ui.checkBox_logfile->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_INDEX)).toBool()
            || ui.checkBox_singlefile->isChecked() != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SF_INDEX)).toBool()
            || playlist_format != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_FORMAT_INDEX)).toString()
            || playlist_scheme != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_NAME_INDEX)).toString()
            || playlist_abs_file_path != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX)).toBool()
            || playlist_utf8 != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_PL_UTF8_INDEX)).toBool()
            || infofile_text != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_INF_TEXT_INDEX)).toStringList()
            || infofile_scheme != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_INF_NAME_INDEX)).toString()
            || infofile_suffix != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_INF_SUFFIX_INDEX)).toString()
            || hashlist_format != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_FORMAT_INDEX)).toString()
            || hashlist_scheme != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_NAME_INDEX)).toString()
            || logfile_scheme != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_NAME_INDEX)).toString()
            || logfile_write_timestamps != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_WRITE_TIMESTAMPS_INDEX)).toBool()
            || singlefile_scheme != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SF_NAME_INDEX)).toString()
            || singlefile_scheme != profile_model->data(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SF_NAME_INDEX)).toString()
            || lame_widget->isChanged() || oggenc_widget->isChanged() || flac_widget->isChanged() || faac_widget->isChanged() || wave_widget->isChanged()
            || custom_widget->isChanged());
    }
}

void ProfileDataDialog::enable_settings_cover(bool enabled)
{
    ui.kpushbutton_cover->setEnabled(enabled);
}

void ProfileDataDialog::enable_settings_playlist(bool enabled)
{
    ui.kpushbutton_playlist->setEnabled(enabled);
}

void ProfileDataDialog::enable_settings_info(bool enabled)
{
    ui.kpushbutton_info->setEnabled(enabled);
}

void ProfileDataDialog::enable_settings_hashlist(bool enabled)
{
    ui.kpushbutton_hashlist->setEnabled(enabled);
}

void ProfileDataDialog::enable_settings_cuesheet(bool enabled)
{
    ui.kpushbutton_cuesheet->setEnabled(enabled);
}

void ProfileDataDialog::enable_settings_logfile(bool enabled)
{
    ui.kpushbutton_logfile->setEnabled(enabled);
}

void ProfileDataDialog::enable_settings_singlefile(bool enabled)
{
    ui.kpushbutton_singlefile->setEnabled(enabled);
}

void ProfileDataDialog::disable_playlist(bool disabled)
{
    ui.checkBox_playlist->setEnabled(!disabled);
    ui.kpushbutton_playlist->setEnabled(!disabled);
}

void ProfileDataDialog::enable_filenames(bool enabled)
{
    ui.groupBox_filenames->setEnabled(enabled);
}

void ProfileDataDialog::disable_filenames(bool disabled)
{
    ui.groupBox_filenames->setEnabled(!disabled);
}

void ProfileDataDialog::scheme_wizard()
{
    SchemeWizardDialog *dialog = new SchemeWizardDialog(ui.qlineedit_scheme->text(), this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    ui.qlineedit_scheme->setText(dialog->scheme);

    delete dialog;

    trigger_changed();
}

void ProfileDataDialog::cover_settings()
{
    ProfileDataCoverDialog *dialog = new ProfileDataCoverDialog(profile_model, profile_row, new_profile_mode, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    delete dialog;
}

void ProfileDataDialog::playlist_settings()
{
    ProfileDataPlaylistDialog *dialog = new ProfileDataPlaylistDialog(profile_model, profile_row, new_profile_mode, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    delete dialog;
}

void ProfileDataDialog::info_settings()
{
    ProfileDataInfoDialog *dialog = new ProfileDataInfoDialog(profile_model, profile_row, new_profile_mode, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    delete dialog;
}

void ProfileDataDialog::hashlist_settings()
{
    ProfileDataHashlistDialog *dialog = new ProfileDataHashlistDialog(profile_model, profile_row, new_profile_mode, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    delete dialog;
}

void ProfileDataDialog::cuesheet_settings()
{
    ProfileDataCueSheetDialog *dialog = new ProfileDataCueSheetDialog(profile_model, profile_row, new_profile_mode, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    delete dialog;
}

void ProfileDataDialog::logfile_settings()
{
    ProfileDataLogFileDialog *dialog = new ProfileDataLogFileDialog(profile_model, profile_row, new_profile_mode, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    delete dialog;
}

void ProfileDataDialog::singlefile_settings()
{
    ProfileDataSingleFileDialog *dialog = new ProfileDataSingleFileDialog(profile_model, profile_row, new_profile_mode, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    delete dialog;

    trigger_changed();
}

void ProfileDataDialog::set_encoder_widget(const EncoderAssistant::Encoder encoder)
{
    switch (encoder) {
    case EncoderAssistant::LAME:
        ui.stackedWidget_encoder->setCurrentWidget(lame_widget);
        break;
    case EncoderAssistant::OGGENC:
        ui.stackedWidget_encoder->setCurrentWidget(oggenc_widget);
        break;
    case EncoderAssistant::OPUSENC:
        ui.stackedWidget_encoder->setCurrentWidget(opusenc_widget);
        break;
    case EncoderAssistant::FLAC:
        ui.stackedWidget_encoder->setCurrentWidget(flac_widget);
        break;
    case EncoderAssistant::FAAC:
        ui.stackedWidget_encoder->setCurrentWidget(faac_widget);
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
    bool success = true;

    error.clear();

    if (success)
        success = lame_widget->save();
    if (!success)
        error = lame_widget->lastError();

    if (success) {
        success = oggenc_widget->save();
        if (!success)
            error = oggenc_widget->lastError();
    }

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
        success = faac_widget->save();
        if (!success)
            error = faac_widget->lastError();
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
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_SELECTED_INDEX),
                                         ui.kcombobox_encoder->itemData(ui.kcombobox_encoder->currentIndex()));
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
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_INF_INDEX), ui.checkBox_info->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_HL_INDEX), ui.checkBox_hashlist->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_CUE_INDEX), ui.checkBox_cuesheet->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_LOG_INDEX), ui.checkBox_logfile->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_SF_INDEX), ui.checkBox_singlefile->isChecked());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_LAME_PARAMETERS_INDEX), lame_parameters.toString());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_OGGENC_PARAMETERS_INDEX), oggenc_parameters.toString());
    if (success)
        success =
            profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_OPUSENC_PARAMETERS_INDEX), opusenc_parameters.toString());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_FLAC_PARAMETERS_INDEX), flac_parameters.toString());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_FAAC_PARAMETERS_INDEX), faac_parameters.toString());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_WAVE_PARAMETERS_INDEX), wave_parameters.toString());
    if (success)
        success = profile_model->setData(profile_model->index(profile_row, PROFILE_MODEL_COLUMN_ENCODER_CUSTOM_PARAMETERS_INDEX), custom_parameters.toString());
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
