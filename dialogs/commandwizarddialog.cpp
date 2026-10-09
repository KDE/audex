/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dialogs/commandwizarddialog.h"
#include "dialogs/textviewdialog.h"
#include "encoding/registry.h"
#include "utils/encodercommand.h"
#include "utils/schemeparser.h"

#include <QDate>
#include <QDialogButtonBox>
#include <QDir>
#include <QMap>
#include <QTime>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace
{

// what the placeholders stand for in the preview
QMap<QString, QString>
exampleValues(const QString &artist, const QString &title, const QString &date, const QString &genre, const QString &cdNo, const QString &tracks)
{
    return {{QStringLiteral(VAR_ALBUM_ARTIST), artist},
            {QStringLiteral(VAR_ALBUM_TITLE), title},
            {QStringLiteral(VAR_DATE), date},
            {QStringLiteral(VAR_GENRE), genre},
            {QStringLiteral(VAR_CD_NO), cdNo},
            {QStringLiteral(VAR_NO_OF_TRACKS), tracks},
            {QStringLiteral(VAR_ENCODER), QStringLiteral("LAME 3.100")},
            {QStringLiteral(VAR_AUDEX), QStringLiteral("Audex")},
            {QStringLiteral(VAR_DISCID), QStringLiteral("a70de90c")},
            {QStringLiteral(VAR_MCN), QStringLiteral("4006381333931")},
            {QStringLiteral(VAR_CD_SIZE), QStringLiteral("587 MiB")},
            {QStringLiteral(VAR_CD_LENGTH), QStringLiteral("55:42.120")},
            {QStringLiteral(VAR_TODAY), QDate::currentDate().toString(Qt::ISODate)},
            {QStringLiteral(VAR_NOW), QTime::currentTime().toString(QStringLiteral("hh-mm-ss"))},
            {QStringLiteral(VAR_LINEBREAK), QStringLiteral(" ")}};
}

}

CommandWizardDialog::CommandWizardDialog(const QString &command, const QString &suffix, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    m_suffix = suffix;

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
    connect(ui.qlineedit_command, &QLineEdit::textEdited, this, &CommandWizardDialog::trigger_changed);
    connect(ui.qlineedit_command, &QLineEdit::textChanged, this, &CommandWizardDialog::update_example);
    ui.qlineedit_command->setCursorPosition(0);

    connect(ui.kurllabel_help, &KUrlLabel::leftClickedUrl, this, &CommandWizardDialog::help);

    connect(ui.kpushbutton_albumartist, &QAbstractButton::clicked, this, &CommandWizardDialog::insAlbumArtist);
    connect(ui.kpushbutton_albumtitle, &QAbstractButton::clicked, this, &CommandWizardDialog::insAlbumTitle);
    connect(ui.kpushbutton_trackartist, &QAbstractButton::clicked, this, &CommandWizardDialog::insTrackArtist);
    connect(ui.kpushbutton_tracktitle, &QAbstractButton::clicked, this, &CommandWizardDialog::insTrackTitle);
    connect(ui.kpushbutton_trackno, &QAbstractButton::clicked, this, &CommandWizardDialog::insTrackNo);
    connect(ui.kpushbutton_cdno, &QAbstractButton::clicked, this, &CommandWizardDialog::insCDNo);
    connect(ui.kpushbutton_date, &QAbstractButton::clicked, this, &CommandWizardDialog::insDate);
    connect(ui.kpushbutton_genre, &QAbstractButton::clicked, this, &CommandWizardDialog::insGenre);
    connect(ui.kpushbutton_nooftracks, &QAbstractButton::clicked, this, &CommandWizardDialog::insNoOfTracks);
    connect(ui.kpushbutton_input_file, &QAbstractButton::clicked, this, &CommandWizardDialog::insInFile);
    connect(ui.kpushbutton_output_file, &QAbstractButton::clicked, this, &CommandWizardDialog::insOutFile);

    this->command = command;

    applyButton->setEnabled(false);

    update_example();
}

CommandWizardDialog::~CommandWizardDialog() = default;

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
    if (!help_dialog) {
        help_dialog = new TextViewDialog(SchemeParser::helpHTMLDoc(1), i18n("Filename scheme help"), this);
    }

    help_dialog->show();
    help_dialog->raise();
    help_dialog->activateWindow();
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
    // both steps, exactly as the engine does them: album values before the
    // rip, track values when the output file is opened; %o is shown with an
    // example path, resolved by the same code the engine uses
    const QString suffix = m_suffix.isEmpty() ? u"m4a"_s : m_suffix;
    const auto preview = [this](const QMap<QString, QString> &album, const QMap<QString, QString> &track, const QString &outputPath) {
        const Audex::Encoding::CommandScheme scheme = Audex::Encoding::parseCommandScheme(ui.qlineedit_command->text(), album);
        const QStringList args = Audex::Encoding::ExternalEncoderFactory::commandLine(Audex::Encoding::substituteValues(scheme.arguments, track), outputPath);
        return qMakePair(Audex::Encoding::commandToString(args), scheme.unsupported);
    };

    const QString musicDir = QDir::homePath() + u"/music/"_s;
    const auto album = preview(exampleValues(QStringLiteral("Meat Loaf"),
                                             QStringLiteral("Bat Out Of Hell III"),
                                             QStringLiteral("2006"),
                                             QStringLiteral("Rock"),
                                             QString(),
                                             QStringLiteral("12")),
                               Audex::Encoding::trackValues(QStringLiteral("Meat Loaf"), QStringLiteral("Blind As A Bat"), 2, QStringLiteral("AA6Q72000047")),
                               musicDir + u"Meat Loaf/Bat Out Of Hell III/02 - Blind As A Bat."_s + suffix);
    ui.qlineedit_album_example->setText(album.first);
    ui.qlineedit_album_example->setCursorPosition(0);

    const auto sampler =
        preview(exampleValues(QStringLiteral("Alternative Hits"),
                              QStringLiteral("Volume 4"),
                              QStringLiteral("2003"),
                              QStringLiteral("Darkwave"),
                              QStringLiteral("2"),
                              QStringLiteral("18")),
                Audex::Encoding::trackValues(QStringLiteral("Wolfsheim"), QStringLiteral("Approaching Lightspeed"), 4, QStringLiteral("DEA123400001")),
                musicDir + u"Alternative Hits/Volume 4/04 - Approaching Lightspeed."_s + suffix);
    ui.qlineedit_sampler_example->setText(sampler.first);
    ui.qlineedit_sampler_example->setCursorPosition(0);

    ui.label_note->setText(album.second.isEmpty()
                               ? i18n("Audex sends the audio to the command as WAVE on standard input (that is what %1 becomes); %2 is the file the command "
                                      "has to write.",
                                      QStringLiteral("$i"),
                                      QStringLiteral("$o"))
                               : i18n("%1 cannot be filled in; a rip with this command is refused.", album.second.join(QStringLiteral(", "))));
}
