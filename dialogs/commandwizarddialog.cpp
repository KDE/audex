/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "commandwizarddialog.h"
#include "dialogs/textviewdialog.h"
#include "utils/schemeparser.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

CommandWizardDialog::CommandWizardDialog(const QString &command, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    setWindowTitle(i18n("Command Scheme Wizard"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    okButton = buttonBox->button(QDialogButtonBox::Ok);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CommandWizardDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CommandWizardDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &CommandWizardDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    help_dialog = new TextViewDialog(SchemeParser::helpHTMLDoc(2), i18n("Command scheme help"), this);

    ui.qlineedit_command->setText(command);
    connect(ui.qlineedit_command, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));
    connect(ui.qlineedit_command, SIGNAL(textChanged(const QString &)), this, SLOT(update_example()));
    ui.qlineedit_command->setCursorPosition(0);

    connect(ui.kurllabel_help, SIGNAL(leftClickedUrl()), this, SLOT(help()));

    connect(ui.kpushbutton_albumartist, SIGNAL(clicked()), this, SLOT(insAlbumArtist()));
    connect(ui.kpushbutton_albumtitle, SIGNAL(clicked()), this, SLOT(insAlbumTitle()));
    connect(ui.kpushbutton_trackartist, SIGNAL(clicked()), this, SLOT(insTrackArtist()));
    connect(ui.kpushbutton_tracktitle, SIGNAL(clicked()), this, SLOT(insTrackTitle()));
    connect(ui.kpushbutton_trackno, SIGNAL(clicked()), this, SLOT(insTrackNo()));
    connect(ui.kpushbutton_cdno, SIGNAL(clicked()), this, SLOT(insCDNo()));
    connect(ui.kpushbutton_date, SIGNAL(clicked()), this, SLOT(insDate()));
    connect(ui.kpushbutton_genre, SIGNAL(clicked()), this, SLOT(insGenre()));
    connect(ui.kpushbutton_cover_file, SIGNAL(clicked()), this, SLOT(insCoverFile()));
    connect(ui.kpushbutton_nooftracks, SIGNAL(clicked()), this, SLOT(insNoOfTracks()));
    connect(ui.kpushbutton_input_file, SIGNAL(clicked()), this, SLOT(insInFile()));
    connect(ui.kpushbutton_output_file, SIGNAL(clicked()), this, SLOT(insOutFile()));

    this->command = command;

    applyButton->setEnabled(false);

    update_example();
}

CommandWizardDialog::~CommandWizardDialog()
{
    if (help_dialog != nullptr) {
        help_dialog->close();
        delete help_dialog;
        help_dialog = nullptr;
    }
}

void CommandWizardDialog::slotAccepted()
{
    save();
    accept();
}

void CommandWizardDialog::slotApplied()
{
    save();
}

void CommandWizardDialog::trigger_changed()
{
    if (ui.qlineedit_command->text() != command) {
        applyButton->setEnabled(true);
        return;
    }
    applyButton->setEnabled(false);
}

void CommandWizardDialog::help()
{
    help_dialog->showNormal();
}

void CommandWizardDialog::insAlbumArtist()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_ALBUM_ARTIST));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insAlbumTitle()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_ALBUM_TITLE));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insTrackArtist()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_TRACK_ARTIST));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insTrackTitle()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_TRACK_TITLE));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insTrackNo()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_TRACK_NO));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insCDNo()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_CD_NO));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insDate()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_DATE));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insGenre()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_GENRE));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insCoverFile()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_COVER_FILE));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insNoOfTracks()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), QString("$" VAR_NO_OF_TRACKS));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insInFile()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_INPUT_FILE));
    ui.qlineedit_command->setText(text);
    update_example();
}

void CommandWizardDialog::insOutFile()
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), '$' + QString(VAR_OUTPUT_FILE));
    ui.qlineedit_command->setText(text);
    update_example();
}

bool CommandWizardDialog::save()
{
    command = ui.qlineedit_command->text();
    applyButton->setEnabled(false);
    return true;
}

void CommandWizardDialog::update_example()
{
    SchemeParser schemeparser;
    QString filename = schemeparser.parsePerTrackCommandScheme(ui.qlineedit_command->text(),
                                                               "/tmp/tmp.wav",
                                                               QString("%1/music/Meat Loaf/02 - Meat Loaf - Blind As A Bat.ogg").arg(QDir::homePath()),
                                                               2,
                                                               1,
                                                               1,
                                                               12,
                                                               "Meat Loaf",
                                                               "Bat Out Of Hell III",
                                                               "Meat Loaf",
                                                               "Blind As A Bat",
                                                               "2006",
                                                               "Rock",
                                                               "AA6Q72000047",
                                                               "ogg",
                                                               QImage(),
                                                               QDir::tempPath(),
                                                               "LAME 3.100",
                                                               true);
    ui.qlineedit_album_example->setText(filename);
    ui.qlineedit_album_example->setCursorPosition(0);
    filename =
        schemeparser.parsePerTrackCommandScheme(ui.qlineedit_command->text(),
                                                "/tmp/tmp.wav",
                                                QString("%1/music/Alternative Hits/Volume 4/04 - Wolfsheim - Approaching Lightspeed.ogg").arg(QDir::homePath()),
                                                4,
                                                2,
                                                1,
                                                18,
                                                "Alternative Hits",
                                                "Volume 4",
                                                "Wolfsheim",
                                                "Approaching Lightspeed",
                                                "2003",
                                                "Darkwave",
                                                "AA6Q72000047",
                                                "ogg",
                                                QImage(),
                                                QDir::tempPath(),
                                                "LAME 3.100",
                                                true);
    ui.qlineedit_sampler_example->setText(filename);
    ui.qlineedit_sampler_example->setCursorPosition(0);
}
