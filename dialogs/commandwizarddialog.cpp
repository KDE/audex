/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "commandwizarddialog.h"
#include "dialogs/textviewdialog.h"
#include "utils/schemeparser.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Audex
{

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
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &CommandWizardDialog::slotAccepted);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &CommandWizardDialog::reject);
    QObject::connect(applyButton, &QPushButton::clicked, this, &CommandWizardDialog::slotApplied);

    auto *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    help_dialog = new TextViewDialog(SchemeParser::helpHTMLDoc(2), i18n("Command scheme help"), this);

    ui.qlineedit_command->setText(command);
    QObject::connect(ui.qlineedit_command, &QLineEdit::textEdited, this, &CommandWizardDialog::trigger_changed);
    QObject::connect(ui.qlineedit_command, &QLineEdit::textChanged, this, &CommandWizardDialog::update_example);
    ui.qlineedit_command->setCursorPosition(0);

    QObject::connect(ui.kurllabel_help, &KUrlLabel::leftClickedUrl, this, &CommandWizardDialog::help);

    QObject::connect(ui.kpushbutton_albumartist, &QPushButton::clicked, this, &CommandWizardDialog::insAlbumArtist);
    QObject::connect(ui.kpushbutton_albumtitle, &QPushButton::clicked, this, &CommandWizardDialog::insAlbumTitle);
    QObject::connect(ui.kpushbutton_trackartist, &QPushButton::clicked, this, &CommandWizardDialog::insTrackArtist);
    QObject::connect(ui.kpushbutton_tracktitle, &QPushButton::clicked, this, &CommandWizardDialog::insTrackTitle);
    QObject::connect(ui.kpushbutton_trackno, &QPushButton::clicked, this, &CommandWizardDialog::insTrackNo);
    QObject::connect(ui.kpushbutton_cdno, &QPushButton::clicked, this, &CommandWizardDialog::insCDNo);
    QObject::connect(ui.kpushbutton_date, &QPushButton::clicked, this, &CommandWizardDialog::insDate);
    QObject::connect(ui.kpushbutton_genre, &QPushButton::clicked, this, &CommandWizardDialog::insGenre);
    QObject::connect(ui.kpushbutton_cover_file, &QPushButton::clicked, this, &CommandWizardDialog::insCoverFile);
    QObject::connect(ui.kpushbutton_nooftracks, &QPushButton::clicked, this, &CommandWizardDialog::insNoOfTracks);
    QObject::connect(ui.kpushbutton_input_file, &QPushButton::clicked, this, &CommandWizardDialog::insInFile);
    QObject::connect(ui.kpushbutton_output_file, &QPushButton::clicked, this, &CommandWizardDialog::insOutFile);

    this->command = command;

    applyButton->setEnabled(false);

    update_example();
}

CommandWizardDialog::~CommandWizardDialog()
{
    if (help_dialog) {
        help_dialog->close();
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

}
