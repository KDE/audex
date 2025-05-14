/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "schemewizarddialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

SchemeWizardDialog::SchemeWizardDialog(const QString &scheme, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    setWindowTitle(i18n("Filename Scheme Wizard"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SchemeWizardDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SchemeWizardDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &SchemeWizardDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    help_dialog = new TextViewDialog(SchemeParser::helpHTMLDoc(1), i18n("Filename scheme help"), this);

    ui.qlineedit_scheme->setText(scheme);
    connect(ui.qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));
    connect(ui.qlineedit_scheme, SIGNAL(textChanged(const QString &)), this, SLOT(update_example()));
    ui.qlineedit_scheme->setCursorPosition(0);

    connect(ui.kurllabel_help, SIGNAL(leftClickedUrl()), this, SLOT(help()));

    connect(ui.kpushbutton_albumartist, SIGNAL(clicked()), this, SLOT(insAlbumArtist()));
    connect(ui.kpushbutton_albumtitle, SIGNAL(clicked()), this, SLOT(insAlbumTitle()));
    connect(ui.kpushbutton_trackartist, SIGNAL(clicked()), this, SLOT(insTrackArtist()));
    connect(ui.kpushbutton_tracktitle, SIGNAL(clicked()), this, SLOT(insTrackTitle()));
    connect(ui.kpushbutton_trackno, SIGNAL(clicked()), this, SLOT(insTrackNo()));
    connect(ui.kpushbutton_cdno, SIGNAL(clicked()), this, SLOT(insCDNo()));
    connect(ui.kpushbutton_date, SIGNAL(clicked()), this, SLOT(insDate()));
    connect(ui.kpushbutton_genre, SIGNAL(clicked()), this, SLOT(insGenre()));
    connect(ui.kpushbutton_suffix, SIGNAL(clicked()), this, SLOT(insSuffix()));
    connect(ui.kpushbutton_nooftracks, SIGNAL(clicked()), this, SLOT(insNoOfTracks()));

    this->scheme = scheme;

    applyButton->setEnabled(false);

    update_example();
}

SchemeWizardDialog::~SchemeWizardDialog()
{
    if (help_dialog != nullptr) {
        help_dialog->close();
        delete help_dialog;
        help_dialog = nullptr;
    }
}

void SchemeWizardDialog::slotAccepted()
{
    save();
    accept();
}

void SchemeWizardDialog::slotApplied()
{
    save();
}

void SchemeWizardDialog::trigger_changed()
{
    if (ui.qlineedit_scheme->text() != scheme) {
        applyButton->setEnabled(true);
        return;
    }
    applyButton->setEnabled(false);
}

void SchemeWizardDialog::help()
{
    help_dialog->showNormal();
}

void SchemeWizardDialog::insAlbumArtist()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_ALBUM_ARTIST));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void SchemeWizardDialog::insAlbumTitle()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_ALBUM_TITLE));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void SchemeWizardDialog::insTrackArtist()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_TRACK_ARTIST));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void SchemeWizardDialog::insTrackTitle()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_TRACK_TITLE));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void SchemeWizardDialog::insTrackNo()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_TRACK_NO));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void SchemeWizardDialog::insCDNo()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_CD_NO));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void SchemeWizardDialog::insDate()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_DATE));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void SchemeWizardDialog::insGenre()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_GENRE));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void SchemeWizardDialog::insSuffix()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_SUFFIX));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void SchemeWizardDialog::insNoOfTracks()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_NO_OF_TRACKS));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

bool SchemeWizardDialog::save()
{
    scheme = ui.qlineedit_scheme->text();
    applyButton->setEnabled(false);
    return true;
}

void SchemeWizardDialog::update_example()
{
    SchemeParser schemeparser;
    QString filename = schemeparser.parsePerTrackFilenameScheme(ui.qlineedit_scheme->text(),
                                                                2,
                                                                1,
                                                                12,
                                                                1,
                                                                "Meat Loaf",
                                                                "Bat Out Of Hell III",
                                                                "Meat Loaf",
                                                                "Blind As A Bat",
                                                                "2006",
                                                                "Rock",
                                                                "AA6Q72000047",
                                                                "ogg",
                                                                false);
    ui.qlineedit_album_example->setText(filename);
    ui.qlineedit_album_example->setCursorPosition(0);
    filename = schemeparser.parsePerTrackFilenameScheme(ui.qlineedit_scheme->text(),
                                                        4,
                                                        2,
                                                        18,
                                                        1,
                                                        "Alternative Hits",
                                                        "Volume 4",
                                                        "Wolfsheim",
                                                        "Kein Zurueck",
                                                        "2003",
                                                        "Darkwave",
                                                        "AA6Q72000047",
                                                        "ogg",
                                                        false);
    ui.qlineedit_sampler_example->setText(filename);
    ui.qlineedit_sampler_example->setCursorPosition(0);
}
