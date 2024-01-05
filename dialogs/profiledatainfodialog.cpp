/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profiledatainfodialog.h"
#include "utils/schemeparser.h"

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

    help_dialog = new TextViewDialog(SchemeParser::helpHTMLDoc(4), i18n("Text info scheme help"), this);

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

    connect(ui.kurllabel_help, SIGNAL(leftClickedUrl()), this, SLOT(help()));

    connect(ui.kpushbutton_load, SIGNAL(clicked()), this, SLOT(load_text()));
    connect(ui.kpushbutton_save, SIGNAL(clicked()), this, SLOT(save_text()));

    applyButton->setEnabled(false);
}

ProfileDataInfoDialog::~ProfileDataInfoDialog()
{
    if (help_dialog != nullptr) {
        help_dialog->close();
        delete help_dialog;
        help_dialog = nullptr;
    }
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
    FilenameSchemeWizardDialog *dialog = new FilenameSchemeWizardDialog(ui.qlineedit_scheme->text(), suffix, this);

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

void ProfileDataInfoDialog::help()
{
    help_dialog->show();
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
