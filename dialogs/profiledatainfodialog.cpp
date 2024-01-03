/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatainfodialog.h"

#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QFileDialog>

ProfileDataInfoDialog::ProfileDataInfoDialog(const QStringList &text, const QString &scheme, const QString &suffix, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    this->text = text;
    this->scheme = scheme;
    this->suffix = suffix;

    setWindowTitle(i18n("Info Settings"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ProfileDataInfoDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ProfileDataInfoDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &ProfileDataInfoDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    connect(ui.kpushbutton_scheme, SIGNAL(clicked()), this, SLOT(scheme_wizard()));
    ui.kpushbutton_scheme->setIcon(QIcon::fromTheme("tools-wizard"));

    ui.ktextedit_text->setPlainText(text.join("\n"));
    connect(ui.ktextedit_text, SIGNAL(textChanged()), this, SLOT(trigger_changed()));

    ui.qlineedit_scheme->setText(scheme);
    connect(ui.qlineedit_scheme, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    ui.qlineedit_suffix->setText(suffix);
    connect(ui.qlineedit_suffix, SIGNAL(textEdited(const QString &)), this, SLOT(trigger_changed()));

    ui.kpushbutton_load->setIcon(QIcon::fromTheme("document-open"));
    ui.kpushbutton_save->setIcon(QIcon::fromTheme("document-save"));

    connect(ui.kurllabel_aboutvariables, SIGNAL(leftClickedUrl()), this, SLOT(about_variables()));

    connect(ui.kpushbutton_load, SIGNAL(clicked()), this, SLOT(load_text()));
    connect(ui.kpushbutton_save, SIGNAL(clicked()), this, SLOT(save_text()));

    applyButton->setEnabled(false);
}

ProfileDataInfoDialog::~ProfileDataInfoDialog()
{
}

void ProfileDataInfoDialog::slotAccepted()
{
    save();
    accept();
}

void ProfileDataInfoDialog::slotApplied()
{
    save();
}

void ProfileDataInfoDialog::scheme_wizard()
{
    SimpleSchemeWizardDialog *dialog = new SimpleSchemeWizardDialog(ui.qlineedit_scheme->text(), suffix, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }

    ui.qlineedit_scheme->setText(dialog->scheme);

    delete dialog;

    trigger_changed();
}

void ProfileDataInfoDialog::trigger_changed()
{
    if (ui.ktextedit_text->toPlainText().split('\n') != text) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.qlineedit_suffix->text() != suffix) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.qlineedit_scheme->text() != scheme) {
        applyButton->setEnabled(true);
        return;
    }
    applyButton->setEnabled(false);
}

void ProfileDataInfoDialog::about_variables()
{
    QWhatsThis::showText(
        ui.kurllabel_aboutvariables->mapToGlobal(ui.kurllabel_aboutvariables->geometry().topLeft()),
        i18n("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
             "<html>"
             "<head>"
             "<style type=\"text/css\">"
             "p, li { white-space: pre-wrap; }"
             "</style>"
             "</head>"
             "<body>"
             "Variables will be replaced by a special value and can even contain parameters.<br />"
             "For example the variable "
             "<div style=\"font-family:monospace; background: #b3c1d6; color: black\"><pre>"
             "$artist"
             "</pre></div>"
             "or the equivalent"
             "<div style=\"font-family:monospace; background: #b3c1d6; color: black\"><pre>"
             "${artist}"
             "</pre></div>"
             "will be replaced by the relevant artist of the cd. Variables may also have attribute, for example:"
             "<div style=\"font-family:monospace; background: #b3c1d6; color: black\"><pre>"
             "${today format=\"yyyy-MM-dd\"}"
             "</pre></div>"
             "This would print the current date. Setting the format will control how this is done. The example (above)"
             "would result int the date being printed as 2010-10-07 (if this was the current date). See below for more details.<br /><br />"
             "You can make use of the following variables:<br />"
             "<table border=1>"
             "<thead>"
             "<tr>"
             "<th>Variable</th><th>Parameter</th><th>Description</th><th>Example</th>"
             "</tr>"
             "</thead>"
             "<tbody>"
             "<tr>"
             "<td>$artist</td><td></td><td>Prints the relevant artist of the extracted cd.</td><td>${artist }</td>"
             "</tr>"
             "<tr>"
             "<td>$title</td><td></td><td>Prints the relevant title of the extracted cd.</td><td></td>"
             "</tr>"
             "<tr>"
             "<td>$date</td><td></td><td>Prints the relevant date (usually release year) of the extracted cd.</td><td></td>"
             "</tr>"
             "<tr>"
             "<td>$genre;</td><td></td><td>Prints the relevant genre of the extracted cd.</td><td></td>"
             "</tr>"
             "<tr>"
             "<td>$size</td><td>iec,precision</td><td>Prints the overall size of all extracted (compressed) music files (incl. the cover). The attribute iec can be one of the following: b, k, m, g. b means byte, k KiB, m MiB and g GiB. "
             "The attribute precision gives the number of decimal places. Default attributes are iec=\"m\" and precision=\"2\"</td><td>${size iec=\"k\" precision=\"2\"}</td>"
             "</tr>"
             "<tr>"
             "<td>$length</td><td></td><td>Prints the relevant overall length of all extracted tracks. The format is min:sec.</td><td></td>"
             "</tr>"
             "<tr>"
             "<td>$nooftracks</td><td></td><td>Prints the total number of extracted tracks.</td><td></td>"
             "</tr>"
             "<tr>"
             "<td>$discid</td><td>base</td><td>Prints the discid of the current cd. The attribute base is the base of the number. The default is 16 (hexadecimal).</td><td>${discid base=\"16\"}</td>"
             "</tr>"
             "<tr>"
             "<td>$today</td><td>format</td><td>Prints the current date. The attribute format specifies the output. <i>Please have a look at the official qt documentation for the format variables.</i></td><td>${today format=\"yyyy-MM-dddd\"}</td>"
             "</tr>"
             "<tr>"
             "<td>$now</td><td>format</td><td>Prints the current date and/or time. The attribute format specifies the output. <i>Please have a look at the official qt documentation for the format variables.</i></td><td>${now format=\"yyyy-MM-dddd hh:mm:ss\"}</td>"
             "</tr>"
             "<tr>"
             "<td>$encoder</td><td></td><td>Prints encoder name and version.</td>"
             "</tr>"
             "<tr>"
             "<td>$audex</td><td></td><td>Prints Audex name and version.</td>"
             "</tr>"
             "<tr>"
             "<td>$br</td><td></td><td>Prints a linebreak.</td><td></td>"
             "</tr>"
             "</tbody>"
             "</table>"
             "</body>"
             "</html>"),
        ui.kurllabel_aboutvariables);
}

void ProfileDataInfoDialog::load_text()
{
    QString filename = QFileDialog::getOpenFileName(this, i18n("Load Text Template"), QDir::homePath(), "*");
    if (!filename.isEmpty()) {
        QFile file(filename);
        if (file.open(QFile::ReadOnly)) {
            QTextStream in(&file);
            ui.ktextedit_text->setPlainText(in.readAll());
            file.close();
        }
    }
}

void ProfileDataInfoDialog::save_text()
{
    QString filename = QFileDialog::getSaveFileName(this, i18n("Save Text Template"), QDir::homePath(), "*");
    if (!filename.isEmpty()) {
        QFile file(filename);
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&file);
            out << ui.ktextedit_text->toPlainText();
            file.close();
        }
    }
}

bool ProfileDataInfoDialog::save()
{
    text = ui.ktextedit_text->toPlainText().split('\n');
    suffix = ui.qlineedit_suffix->text();
    scheme = ui.qlineedit_scheme->text();
    applyButton->setEnabled(false);
    return true;
}
