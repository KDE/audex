/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatahookwizarddialog.h"

#include "utils/encodercommand.h"
#include "utils/schemeparser.h"

#include <QDate>
#include <QDialogButtonBox>
#include <QDir>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

ProfileDataHookWizardDialog::ProfileDataHookWizardDialog(const QString &command, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    setWindowTitle(i18n("Command Scheme Wizard"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataHookWizardDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataHookWizardDialog::reject);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    ui.qlineedit_command->setText(command);
    ui.qlineedit_command->setCursorPosition(0);
    connect(ui.qlineedit_command, &QLineEdit::textChanged, this, &ProfileDataHookWizardDialog::update_example);

    connect(ui.kpushbutton_albumartist, &QAbstractButton::clicked, this, &ProfileDataHookWizardDialog::insAlbumArtist);
    connect(ui.kpushbutton_albumtitle, &QAbstractButton::clicked, this, &ProfileDataHookWizardDialog::insAlbumTitle);
    connect(ui.kpushbutton_date, &QAbstractButton::clicked, this, &ProfileDataHookWizardDialog::insDate);
    connect(ui.kpushbutton_genre, &QAbstractButton::clicked, this, &ProfileDataHookWizardDialog::insGenre);
    connect(ui.kpushbutton_nooftracks, &QAbstractButton::clicked, this, &ProfileDataHookWizardDialog::insNoOfTracks);
    connect(ui.kpushbutton_today, &QAbstractButton::clicked, this, &ProfileDataHookWizardDialog::insToday);
    connect(ui.kpushbutton_outputdir, &QAbstractButton::clicked, this, &ProfileDataHookWizardDialog::insOutputDir);
    connect(ui.kpushbutton_filelist, &QAbstractButton::clicked, this, &ProfileDataHookWizardDialog::insFileList);

    this->command = command;

    update_example();
}

void ProfileDataHookWizardDialog::slotAccepted()
{
    command = ui.qlineedit_command->text();
    accept();
}

void ProfileDataHookWizardDialog::insert(const QString &variable)
{
    QString text = ui.qlineedit_command->text();
    text.insert(ui.qlineedit_command->cursorPosition(), variable);
    ui.qlineedit_command->setText(text);
    ui.qlineedit_command->setFocus();
}

void ProfileDataHookWizardDialog::insAlbumArtist()
{
    insert(u"$"_s + QStringLiteral(VAR_ALBUM_ARTIST));
}

void ProfileDataHookWizardDialog::insAlbumTitle()
{
    insert(u"$"_s + QStringLiteral(VAR_ALBUM_TITLE));
}

void ProfileDataHookWizardDialog::insDate()
{
    insert(u"$"_s + QStringLiteral(VAR_DATE));
}

void ProfileDataHookWizardDialog::insGenre()
{
    insert(u"$"_s + QStringLiteral(VAR_GENRE));
}

void ProfileDataHookWizardDialog::insNoOfTracks()
{
    insert(u"$"_s + QStringLiteral(VAR_NO_OF_TRACKS));
}

void ProfileDataHookWizardDialog::insToday()
{
    insert(u"$"_s + QStringLiteral(VAR_TODAY));
}

void ProfileDataHookWizardDialog::insOutputDir()
{
    insert(QStringLiteral("%d"));
}

void ProfileDataHookWizardDialog::insFileList()
{
    insert(QStringLiteral("%f"));
}

void ProfileDataHookWizardDialog::update_example()
{
    const QString dir = QDir::homePath() + u"/music/Meat Loaf/Bat Out Of Hell III"_s;

    // the values the rip would fill in; the preview runs the same code as
    // the execution, so what is shown is what runs
    const QMap<QString, QString> album = {{QStringLiteral(VAR_NO_OF_TRACKS), u"12"_s},
                                          {QStringLiteral(VAR_ALBUM_ARTIST), u"Meat Loaf"_s},
                                          {QStringLiteral(VAR_ALBUM_TITLE), u"Bat Out Of Hell III"_s},
                                          {QStringLiteral(VAR_DATE), u"2006"_s},
                                          {QStringLiteral(VAR_GENRE), u"Rock"_s},
                                          {QStringLiteral(VAR_CD_NO), QString()},
                                          {QStringLiteral(VAR_TODAY), QDate::currentDate().toString(Qt::ISODate)}};
    const QStringList files{dir + u"/01 - Bat Out Of Hell.ogg"_s, dir + u"/02 - Blind As A Bat.ogg"_s};

    const QStringList args = Audex::Encoding::hookCommandArguments(ui.qlineedit_command->text(), album, files, dir);
    ui.qlineedit_example->setText(Audex::Encoding::commandToString(args));
    ui.qlineedit_example->setCursorPosition(0);
}
