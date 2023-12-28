/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "commandwizarddialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

CommandWizardDialog::CommandWizardDialog(const QString &command, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    setWindowTitle(i18n("Command Pattern Wizard"));

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

    ui.qlineedit_command->setText(command);
    connect(ui.qlineedit_command, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));
    connect(ui.qlineedit_command, SIGNAL(textChanged(const QString &)), this, SLOT(update_example()));
    ui.qlineedit_command->setCursorPosition(0);

    connect(ui.kurllabel_aboutcommandlineschemes, SIGNAL(leftClickedUrl()), this, SLOT(about_commandline_schemes()));
    connect(ui.kurllabel_aboutparameters, SIGNAL(leftClickedUrl()), this, SLOT(about_parameters()));

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

void CommandWizardDialog::about_commandline_schemes()
{
    QWhatsThis::showText(
        ui.kurllabel_aboutcommandlineschemes->mapToGlobal(ui.kurllabel_aboutcommandlineschemes->geometry().topLeft()),
        i18n("<p>The following variables will be replaced with their particular meaning in every track name.</p>"
             "<p><table border=\"1\">"
             "<tr><th><em>Variable</em></th><th><em>Description</em></th></tr>"
             "<tr><td>$artist</td><td>The artist of the CD. If your CD is a compilation, then this tag represents the title in most cases.</td></tr>"
             "<tr><td>$title</td><td>The title of the CD. If your CD is a compilation, then this tag represents the subtitle in most cases.</td></tr>"
             "<tr><td>$date</td><td>The release date of the CD. In almost all cases this is the year.</td></tr>"
             "<tr><td>$genre</td><td>The genre of the CD.</td></tr>"
             "<tr><td>$cdno</td><td>The CD number of a multi-CD album. Often compilations consist of several CDs. <i>Note:</i> If the multi-CD flag is "
             "<b>not</b> set for the current CD, than this value will be just empty.</td></tr>"
             "<tr><td>$tartist</td><td>This is the artist of every individual track. It is especially useful on compilation CDs.</td></tr>"
             "<tr><td>$ttitle</td><td>The track title. Normally each track on a CD has its own title, which is the name of the song.</td></tr>"
             "<tr><td>$trackno</td><td>The track number. First track is 1.</td></tr>"
             "<tr><td>$cover</td><td>The cover file.</td></tr>"
             "<tr><td>$nooftracks</td><td>The total number of audio tracks of the CD.</td></tr>"
             "<tr><td>$i</td><td>The temporary WAV file (input file) created by Audex from CD audio track. You can use it as a normal input file for your "
             "command line encoder.</td></tr>"
             "<tr><td>$o</td><td>The full output filename and path (output file). Use it as the output for your command line encoder.</td></tr>"
             "<tr><td>$encoder</td><td>Encoder name and version.</td></tr>"
             "<tr><td>$audex</td><td>Audex name and version.</td></tr>"
             "</table></p>"),
        ui.kurllabel_aboutcommandlineschemes);
}

void CommandWizardDialog::about_parameters()
{
    QWhatsThis::showText(ui.kurllabel_aboutparameters->mapToGlobal(ui.kurllabel_aboutparameters->geometry().topLeft()),
                         i18n("<p>Variables in Audex can have parameters. E.g.</p>"
                              "<pre>${cover format=\"JPG\" x=\"300\" y=\"300\" preparam=\"-ti \"}</pre>"
                              "<p>In this example a temporary filename of a copy in jpeg format and size"
                              "of 300x300 pixels of the cover will be inserted."
                              "If no size is set, the size of the original cover image file will be taken."
                              "If no cover is set, this variable will be omitted."
                              "Possible formats are \"JPG\", \"PNG\" and \"GIF\" (Default: \"JPG\").</p>\n"
                              "<p><i><b>Note:</b> LAME discards cover files larger than 128 KiB.</i></p>"
                              "<hr />"
                              "<p>\"preparam\" and \"postparam\" define parameters inserted before "
                              "(pre) or behind (post) the variable. These values are <b>only</b>"
                              "shown if a value is set. Works with all commandline variables."),
                         ui.kurllabel_aboutparameters);
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
    PatternParser patternparser;
    QString filename = patternparser.parseCommandPattern(ui.qlineedit_command->text(),
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
                                                         "ogg",
                                                         QImage(),
                                                         false,
                                                         QDir::tempPath(),
                                                         "LAME 3.98.2",
                                                         true);
    ui.qlineedit_album_example->setText(filename);
    ui.qlineedit_album_example->setCursorPosition(0);
    filename =
        patternparser.parseCommandPattern(ui.qlineedit_command->text(),
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
                                          "ogg",
                                          QImage(),
                                          false,
                                          QDir::tempPath(),
                                          "LAME 3.98.2",
                                          true);
    ui.qlineedit_sampler_example->setText(filename);
    ui.qlineedit_sampler_example->setCursorPosition(0);
}
