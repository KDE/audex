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

    ui.qlineedit_scheme->setText(scheme);
    connect(ui.qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));
    connect(ui.qlineedit_scheme, SIGNAL(textChanged(const QString &)), this, SLOT(update_example()));
    ui.qlineedit_scheme->setCursorPosition(0);

    connect(ui.kurllabel_aboutfilenameschemes, SIGNAL(leftClickedUrl()), this, SLOT(about_filename_schemes()));
    connect(ui.kurllabel_aboutparameters, SIGNAL(leftClickedUrl()), this, SLOT(about_parameters()));

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

void SchemeWizardDialog::about_filename_schemes()
{
    QWhatsThis::showText(
        ui.kurllabel_aboutfilenameschemes->mapToGlobal(ui.kurllabel_aboutfilenameschemes->geometry().topLeft()),
        i18n("<p>The following variables will be replaced with their particular meaning in every track name.</p>"
             "<p><table border=\"1\">"
             "<tr><th><em>Variable</em></th><th><em>Description</em></th></tr>"
             "<tr><td>$artist</td><td>The artist of the CD. If your CD is a compilation, then this tag represents the title in most cases.</td></tr>"
             "<tr><td>$title</td><td>The title of the CD. If your CD is a compilation, then this tag represents the subtitle in most cases.</td></tr>"
             "<tr><td>$date</td><td>The release date of the CD. In almost all cases this is the year.</td></tr>"
             "<tr><td>$genre</td><td>The genre of the CD.</td></tr>"
             "<tr><td>$cdno</td><td>The CD number of a multi-CD album. Often compilations consist of several CDs. <i>Note:</i> If the multi-CD flag is <b>not</b> set for the current CD, than this value will be just empty.</td></tr>"
             "<tr><td>$tartist</td><td>This is the artist of every individual track. It is especially useful on compilation CDs.</td></tr>"
             "<tr><td>$ttitle</td><td>The track title. Normally each track on a CD has its own title, which is the name of the song.</td></tr>"
             "<tr><td>$trackno</td><td>The track number. First track is 1.</td></tr>"
             "<tr><td>$nooftracks</td><td>The total number of audio tracks of the CD.</td></tr>"
             "<tr><td>$suffix</td><td>The filename extension.</td></tr>"
             "<tr><td>$audex</td><td>Audex name and version.</td></tr>"
             "</table></p>"),
        ui.kurllabel_aboutfilenameschemes);
}

void SchemeWizardDialog::about_parameters()
{
    QWhatsThis::showText(ui.kurllabel_aboutparameters->mapToGlobal(ui.kurllabel_aboutparameters->geometry().topLeft()),
                         i18n("<p>Variables in Audex can have parameters. E.g.</p>"
                              "<pre>$artist/$title/${trackno length=\"2\" fillchar=\"0\"} - $ttile.$suffix</pre>"
                              "<p>This means, that the tracknumber will be forced to a length of 2 digits. If the number is less than 10, it will be filled up with \"0\".</p>"
                              "<pre>$artist/${title lowercase=\"true\"}/$trackno - $ttile.$suffix<br />"
                              "$artist/${title uppercase=\"true\"}/$trackno - $ttile.$suffix</pre>"
                              "<p>The title will be uppercased or lowercased. This works with the other variables, too.</p>"
                              "<pre>${artist left=\"1\"}/$artist/$title/$trackno - $ttile.$suffix</pre>"
                              "<p>Take one character from the left, which is the first one.</p>"
                              "<hr />"
                              "<p><b><i>To have a complete overview of parameters go to the Audex webpage.</i></b></p>"),
                         ui.kurllabel_aboutparameters);
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
    QString filename = schemeparser.parseFilenameScheme(ui.qlineedit_scheme->text(), 2, 1, 12, 1, "Meat Loaf", "Bat Out Of Hell III", "Meat Loaf", "Blind As A Bat", "2006", "Rock", "ogg", false, false, false);
    ui.qlineedit_album_example->setText(filename);
    ui.qlineedit_album_example->setCursorPosition(0);
    filename = schemeparser.parseFilenameScheme(ui.qlineedit_scheme->text(), 4, 2, 18, 1, "Alternative Hits", "Volume 4", "Wolfsheim", "Kein Zurueck", "2003", "Darkwave", "ogg", false, false, false);
    ui.qlineedit_sampler_example->setText(filename);
    ui.qlineedit_sampler_example->setCursorPosition(0);
}