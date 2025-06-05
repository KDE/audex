/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "schemewizarddialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Audex
{

SchemeWizardDialog::SchemeWizardDialog(const QString &scheme, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    setWindowTitle(i18n("Filename Scheme Wizard"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    auto *okButton = buttonBox->button(QDialogButtonBox::Ok);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &SchemeWizardDialog::slotAccepted);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &SchemeWizardDialog::reject);
    QObject::connect(applyButton, &QPushButton::clicked, this, &SchemeWizardDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    help_dialog = new TextViewDialog(SchemeParser::helpHTMLDoc(1), i18n("Filename scheme help"), this);

    ui.qlineedit_scheme->setText(scheme);
    QObject::connect(ui.qlineedit_scheme, &QLineEdit::textEdited, this, &SchemeWizardDialog::trigger_changed);
    QObject::connect(ui.qlineedit_scheme, &QLineEdit::textChanged, this, &SchemeWizardDialog::update_example);

    ui.qlineedit_scheme->setCursorPosition(0);

    QObject::connect(ui.kurllabel_help, &KUrlLabel::leftClickedUrl, this, &SchemeWizardDialog::help);

    QObject::connect(ui.kpushbutton_albumartist, &QPushButton::clicked, this, &SchemeWizardDialog::insAlbumArtist);
    QObject::connect(ui.kpushbutton_albumtitle, &QPushButton::clicked, this, &SchemeWizardDialog::insAlbumTitle);
    QObject::connect(ui.kpushbutton_trackartist, &QPushButton::clicked, this, &SchemeWizardDialog::insTrackArtist);
    QObject::connect(ui.kpushbutton_tracktitle, &QPushButton::clicked, this, &SchemeWizardDialog::insTrackTitle);
    QObject::connect(ui.kpushbutton_trackno, &QPushButton::clicked, this, &SchemeWizardDialog::insTrackNo);
    QObject::connect(ui.kpushbutton_cdno, &QPushButton::clicked, this, &SchemeWizardDialog::insCDNo);
    QObject::connect(ui.kpushbutton_date, &QPushButton::clicked, this, &SchemeWizardDialog::insDate);
    QObject::connect(ui.kpushbutton_genre, &QPushButton::clicked, this, &SchemeWizardDialog::insGenre);
    QObject::connect(ui.kpushbutton_suffix, &QPushButton::clicked, this, &SchemeWizardDialog::insSuffix);
    QObject::connect(ui.kpushbutton_nooftracks, &QPushButton::clicked, this, &SchemeWizardDialog::insNoOfTracks);

    this->scheme = scheme;

    applyButton->setEnabled(false);

    update_example();
}

SchemeWizardDialog::~SchemeWizardDialog()
{
    if (help_dialog) {
        help_dialog->close();
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

}
