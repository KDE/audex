/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaheaderdatadialog.h"

#include "utils/discidcalculator.h"

#define GENRE_MAX 148
static const char *ID3_GENRES[GENRE_MAX] = {"Blues",
                                            "Classic Rock",
                                            "Country",
                                            "Dance",
                                            "Disco",
                                            "Funk",
                                            "Grunge",
                                            "Hip-Hop",
                                            "Jazz",
                                            "Metal",
                                            "New Age",
                                            "Oldies",
                                            "Other",
                                            "Pop",
                                            "R&B",
                                            "Rap",
                                            "Reggae",
                                            "Rock",
                                            "Techno",
                                            "Industrial",
                                            "Alternative",
                                            "Ska",
                                            "Death Metal",
                                            "Pranks",
                                            "Soundtrack",
                                            "Euro-Techno",
                                            "Ambient",
                                            "Trip-Hop",
                                            "Vocal",
                                            "Jazz+Funk",
                                            "Fusion",
                                            "Trance",
                                            "Classical",
                                            "Instrumental",
                                            "Acid",
                                            "House",
                                            "Game",
                                            "Sound Clip",
                                            "Gospel",
                                            "Noise",
                                            "Alt",
                                            "Bass",
                                            "Soul",
                                            "Punk",
                                            "Space",
                                            "Meditative",
                                            "Instrumental Pop",
                                            "Instrumental Rock",
                                            "Ethnic",
                                            "Gothic",
                                            "Darkwave",
                                            "Techno-Industrial",
                                            "Electronic",
                                            "Pop-Folk",
                                            "Eurodance",
                                            "Dream",
                                            "Southern Rock",
                                            "Comedy",
                                            "Cult",
                                            "Gangsta Rap",
                                            "Top 40",
                                            "Christian Rap",
                                            "Pop/Funk",
                                            "Jungle",
                                            "Native American",
                                            "Cabaret",
                                            "New Wave",
                                            "Psychedelic",
                                            "Rave",
                                            "Showtunes",
                                            "Trailer",
                                            "Lo-Fi",
                                            "Tribal",
                                            "Acid Punk",
                                            "Acid Jazz",
                                            "Polka",
                                            "Retro",
                                            "Musical",
                                            "Rock & Roll",
                                            "Hard Rock",
                                            "Folk",
                                            "Folk/Rock",
                                            "National Folk",
                                            "Swing",
                                            "Fast-Fusion",
                                            "Bebob",
                                            "Latin",
                                            "Revival",
                                            "Celtic",
                                            "Bluegrass",
                                            "Avantgarde",
                                            "Gothic Rock",
                                            "Progressive Rock",
                                            "Psychedelic Rock",
                                            "Symphonic Rock",
                                            "Slow Rock",
                                            "Big Band",
                                            "Chorus",
                                            "Easy Listening",
                                            "Acoustic",
                                            "Humour",
                                            "Speech",
                                            "Chanson",
                                            "Opera",
                                            "Chamber Music",
                                            "Sonata",
                                            "Symphony",
                                            "Booty Bass",
                                            "Primus",
                                            "Porn Groove",
                                            "Satire",
                                            "Slow Jam",
                                            "Club",
                                            "Tango",
                                            "Samba",
                                            "Folklore",
                                            "Ballad",
                                            "Power Ballad",
                                            "Rhythmic Soul",
                                            "Freestyle",
                                            "Duet",
                                            "Punk Rock",
                                            "Drum Solo",
                                            "A Cappella",
                                            "Euro-House",
                                            "Dance Hall",
                                            "Goa",
                                            "Drum & Bass",
                                            "Club-House",
                                            "Hardcore",
                                            "Terror",
                                            "Indie",
                                            "BritPop",
                                            "Negerpunk",
                                            "Polsk Punk",
                                            "Beat",
                                            "Christian Gangsta Rap",
                                            "Heavy Metal",
                                            "Black Metal",
                                            "Crossover",
                                            "Contemporary Christian",
                                            "Christian Rock",
                                            "Merengue",
                                            "Salsa",
                                            "Thrash Metal",
                                            "Anime",
                                            "JPop",
                                            "Synthpop"};

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

    QStringList genres;
    for (int i = 0; i < GENRE_MAX; ++i)
        genres.append(QString().fromLatin1(ID3_GENRES[i]));
    genres.sort();
    KCompletion *comp = ui.kcombobox_genre->completionObject();
    comp->insertItems(genres);
    ui.kcombobox_genre->addItems(genres);
    QObject::connect(ui.kcombobox_genre->lineEdit(), &QLineEdit::returnPressed, this, [=]() {
        comp->addItem(ui.kcombobox_genre->lineEdit()->text());
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

    ui.qlineedit_title->setText(metadata.get(Audex::Metadata::Type::Title).toString());
    QObject::connect(ui.qlineedit_title, &QLineEdit::textEdited, [=](const QString &text) {
        p_metadata.set(Audex::Metadata::Type::Title, text);
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

const Audex::Metadata::Metadata &CDDAHeaderDataDialog::metadata() const
{
    return p_metadata;
}

}
