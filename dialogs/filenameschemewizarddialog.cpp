/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "filenameschemewizarddialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Audex
{

FilenameSchemeWizardDialog::FilenameSchemeWizardDialog(const QString &scheme, const QString &suffix, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    setWindowTitle(i18n("Scheme Wizard"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    auto *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &FilenameSchemeWizardDialog::slotAccepted);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &FilenameSchemeWizardDialog::reject);
    QObject::connect(applyButton, &QPushButton::clicked, this, &FilenameSchemeWizardDialog::slotApplied);

    auto *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    help_dialog = new TextViewDialog(SchemeParser::helpHTMLDoc(3), i18n("Filename scheme help"), this);

    ui.qlineedit_scheme->setText(scheme);
    QObject::connect(ui.qlineedit_scheme, &QLineEdit::textEdited, this, &FilenameSchemeWizardDialog::trigger_changed);
    QObject::connect(ui.qlineedit_scheme, &QLineEdit::textChanged, this, &FilenameSchemeWizardDialog::update_example);
    ui.qlineedit_scheme->setCursorPosition(0);

    QObject::connect(ui.kurllabel_help, &KUrlLabel::leftClickedUrl, this, &FilenameSchemeWizardDialog::help);

    QObject::connect(ui.kpushbutton_albumartist, &QPushButton::clicked, this, &FilenameSchemeWizardDialog::insAlbumArtist);
    QObject::connect(ui.kpushbutton_albumtitle, &QPushButton::clicked, this, &FilenameSchemeWizardDialog::insAlbumTitle);
    QObject::connect(ui.kpushbutton_cdno, &QPushButton::clicked, this, &FilenameSchemeWizardDialog::insCDNo);
    QObject::connect(ui.kpushbutton_date, &QPushButton::clicked, this, &FilenameSchemeWizardDialog::insDate);
    QObject::connect(ui.kpushbutton_genre, &QPushButton::clicked, this, &FilenameSchemeWizardDialog::insGenre);
    QObject::connect(ui.kpushbutton_suffix, &QPushButton::clicked, this, &FilenameSchemeWizardDialog::insSuffix);
    QObject::connect(ui.kpushbutton_nooftracks, &QPushButton::clicked, this, &FilenameSchemeWizardDialog::insNoOfTracks);

    this->scheme = scheme;
    this->suffix = suffix;

    applyButton->setEnabled(false);
    update_example();
}

FilenameSchemeWizardDialog::~FilenameSchemeWizardDialog()
{
    if (help_dialog) {
        help_dialog->close();
    }
}

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
    help_dialog->showNormal();
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

}
