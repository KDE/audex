/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "filenameschemewizarddialog.h"

#include "utils/schemeparser.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

FilenameSchemeWizardDialog::FilenameSchemeWizardDialog(const QString &scheme, const QString &suffix, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    setWindowTitle(i18n("Scheme Wizard"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &FilenameSchemeWizardDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &FilenameSchemeWizardDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &FilenameSchemeWizardDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    help_dialog = new TextViewDialog(SchemeParser::helpHTMLDoc(3), i18n("Filename scheme help"), this);

    ui.qlineedit_scheme->setText(scheme);
    connect(ui.qlineedit_scheme, &QLineEdit::textEdited, this, &FilenameSchemeWizardDialog::trigger_changed);
    connect(ui.qlineedit_scheme, &QLineEdit::textChanged, this, &FilenameSchemeWizardDialog::update_example);
    ui.qlineedit_scheme->setCursorPosition(0);

    connect(ui.kurllabel_help, &KUrlLabel::leftClickedUrl, this, &FilenameSchemeWizardDialog::help);

    connect(ui.kpushbutton_albumartist, &QAbstractButton::clicked, this, &FilenameSchemeWizardDialog::insAlbumArtist);
    connect(ui.kpushbutton_albumtitle, &QAbstractButton::clicked, this, &FilenameSchemeWizardDialog::insAlbumTitle);
    connect(ui.kpushbutton_cdno, &QAbstractButton::clicked, this, &FilenameSchemeWizardDialog::insCDNo);
    connect(ui.kpushbutton_date, &QAbstractButton::clicked, this, &FilenameSchemeWizardDialog::insDate);
    connect(ui.kpushbutton_genre, &QAbstractButton::clicked, this, &FilenameSchemeWizardDialog::insGenre);
    connect(ui.kpushbutton_suffix, &QAbstractButton::clicked, this, &FilenameSchemeWizardDialog::insSuffix);
    connect(ui.kpushbutton_nooftracks, &QAbstractButton::clicked, this, &FilenameSchemeWizardDialog::insNoOfTracks);

    this->scheme = scheme;
    this->suffix = suffix;

    applyButton->setEnabled(false);
    update_example();
}

FilenameSchemeWizardDialog::~FilenameSchemeWizardDialog() = default;

void FilenameSchemeWizardDialog::slotAccepted()
{
    save();
    accept();
}

void FilenameSchemeWizardDialog::slotApplied()
{
    save();
}

void FilenameSchemeWizardDialog::trigger_changed()
{
    if (ui.qlineedit_scheme->text() != scheme) {
        applyButton->setEnabled(true);
        return;
    }
    applyButton->setEnabled(false);
}

void FilenameSchemeWizardDialog::help()
{
    if (!help_dialog) {
        help_dialog = new TextViewDialog(SchemeParser::helpHTMLDoc(1), i18n("Filename scheme help"), this);
    }

    help_dialog->show();
    help_dialog->raise();
    help_dialog->activateWindow();
}

void FilenameSchemeWizardDialog::insAlbumArtist()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_ALBUM_ARTIST));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void FilenameSchemeWizardDialog::insAlbumTitle()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_ALBUM_TITLE));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void FilenameSchemeWizardDialog::insCDNo()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_CD_NO));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void FilenameSchemeWizardDialog::insDate()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_DATE));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void FilenameSchemeWizardDialog::insGenre()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_GENRE));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void FilenameSchemeWizardDialog::insSuffix()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_SUFFIX));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

void FilenameSchemeWizardDialog::insNoOfTracks()
{
    QString text = ui.qlineedit_scheme->text();
    text.insert(ui.qlineedit_scheme->cursorPosition(), QString("$" VAR_NO_OF_TRACKS));
    ui.qlineedit_scheme->setText(text);
    update_example();
}

bool FilenameSchemeWizardDialog::save()
{
    scheme = ui.qlineedit_scheme->text();
    applyButton->setEnabled(false);
    return true;
}

void FilenameSchemeWizardDialog::update_example()
{
    SchemeParser schemeparser;
    QString filename = schemeparser.parseFilenameScheme(ui.qlineedit_scheme->text(), 1, 12, "Meat Loaf", "Bat Out Of Hell III", "2006", "Rock", suffix);
    ui.qlineedit_example->setText(filename);
    ui.qlineedit_example->setCursorPosition(0);
}
