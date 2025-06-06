/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaheaderdatadialog.h"

#include "utils/discidcalculator.h"

namespace Audex
{

CDDAHeaderDataDialog::CDDAHeaderDataDialog(const Audex::Metadata::Metadata &metadata, const Audex::Toc::Toc &toc, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    p_metadata = metadata;
    p_toc = toc;

    setWindowTitle(i18n("Edit Data"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &CDDAHeaderDataDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &CDDAHeaderDataDialog::reject);

    auto *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    KConfig config;
    KConfigGroup group = config.group("Genres");
    config.sync();

    KCompletion *comp = ui.kcombobox_genre->completionObject();
    comp->setOrder(KCompletion::Sorted);
    comp->setIgnoreCase(true);

    {
        QStringList genres = group.readEntry("genres", QStringList());
        genres.sort();
        comp->insertItems(genres);
        ui.kcombobox_genre->addItems(genres);
    }

    ui.kcombobox_genre->setAutoCompletion(true);
    ui.kcombobox_genre->setEditable(true);
    QObject::connect(ui.kcombobox_genre->lineEdit(), &QLineEdit::editingFinished, this, [=]() {
        const QString genre = ui.kcombobox_genre->lineEdit()->text();
        if (!comp->items().contains(genre, Qt::CaseInsensitive)) {
            comp->addItem(genre);
            ui.kcombobox_genre->addItem(genre);
        }
    });

    ui.checkBox_various->setChecked(p_metadata.isVarious());
    QObject::connect(ui.checkBox_various, &QCheckBox::toggled, [=](bool checked) {
        p_metadata.setVarious(checked);
    });

    ui.checkBox_multicd->setChecked(p_metadata.isMultiDisc());
    QObject::connect(ui.checkBox_multicd, &QCheckBox::toggled, [=](bool checked) {
        p_metadata.setMultiDisc(checked);
        ui.kintspinbox_cdnum->setEnabled(checked);
        ui.label_cdnum->setEnabled(checked);
    });

    ui.qlineedit_artist->setText(metadata.get(Audex::Metadata::Type::Artist).toString());
    QObject::connect(ui.qlineedit_artist, &QLineEdit::textEdited, [=](const QString &text) {
        p_metadata.set(Audex::Metadata::Type::Artist, text);
    });

    ui.qlineedit_title->setText(metadata.get(Audex::Metadata::Type::Album).toString());
    QObject::connect(ui.qlineedit_title, &QLineEdit::textEdited, [=](const QString &text) {
        p_metadata.set(Audex::Metadata::Type::Album, text);
    });

    ui.kintspinbox_cdnum->setValue(metadata.discNum());
    QObject::connect(ui.kintspinbox_cdnum, &QSpinBox::valueChanged, [=](int i) {
        p_metadata.setDiscNum(i);
    });

    ui.kintspinbox_trackoffset->setValue(metadata.get(Audex::Metadata::Type::TrackNumberOffset).toInt());
    QObject::connect(ui.kintspinbox_trackoffset, &QSpinBox::valueChanged, [=](int i) {
        p_metadata.set(Audex::Metadata::Type::TrackNumberOffset, i);
    });

    ui.kcombobox_genre->lineEdit()->setText(metadata.get(Audex::Metadata::Type::Genre).toString());
    QObject::connect(ui.kcombobox_genre->lineEdit(), &QLineEdit::textEdited, [=](const QString &text) {
        p_metadata.set(Audex::Metadata::Type::Genre, text);
    });

    {
        bool ok;
        int year = metadata.get(Audex::Metadata::Type::Year).toInt(&ok);
        if (ok)
            ui.kintspinbox_year->setValue(year);
        else
            ui.kintspinbox_year->setValue(QDate::currentDate().year());
    }
    ui.kintspinbox_cdnum->setValue(metadata.discNum());
    QObject::connect(ui.kintspinbox_year, &QSpinBox::valueChanged, [=](int i) {
        p_metadata.set(Audex::Metadata::Type::Year, i);
    });

    ui.ktextedit_extdata->setText(metadata.get(Audex::Metadata::Type::Comment).toString());
    QObject::connect(ui.ktextedit_extdata, &KTextEdit::textChanged, [=]() {
        p_metadata.set(Audex::Metadata::Type::Comment, ui.ktextedit_extdata->toPlainText());
    });

    ui.qlineedit_cddbdiscid->setText(QString("0x%1").arg(DiscIDCalculator::CDDBId(p_toc.discSignature()), 0, 16));

    ui.kintspinbox_cdnum->setEnabled(metadata.isMultiDisc());
    ui.label_cdnum->setEnabled(metadata.isMultiDisc());
}

CDDAHeaderDataDialog::~CDDAHeaderDataDialog()
{
    KConfig config;
    KConfigGroup group = config.group("Genres");
    group.writeEntry("genres", ui.kcombobox_genre->completionObject()->items());
    config.sync();
}

const Audex::Metadata::Metadata &CDDAHeaderDataDialog::metadata() const
{
    return p_metadata;
}

}
