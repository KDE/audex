/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaheaderdatadialog.h"

#include "utils/genres.h"
#include "widgets/cddaheaderwidget.h"

#include <QDate>
#include <QDialogButtonBox>
#include <QEvent>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

#include <KCompletion>
#include <KLocalizedString>

using namespace Qt::StringLiterals;

CDDAHeaderDataDialog::CDDAHeaderDataDialog(Audex::CDInfoModel *cddaModel, std::optional<bool> hdcd, QWidget *parent)
    : QDialog(parent)
{
    Q_UNUSED(parent);

    cdda_model = cddaModel;
    if (!cdda_model) {
        qWarning() << "CDDAHeaderDataDialog() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    setWindowTitle(i18n("Edit Data"));

    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    okButton = buttonBox->button(QDialogButtonBox::Ok);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CDDAHeaderDataDialog::slotAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CDDAHeaderDataDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &CDDAHeaderDataDialog::slotApplied);

    QWidget *widget = new QWidget(this);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(widget);

    const QStringList genres = Audex::Genres::presets();
    KCompletion *comp = ui.kcombobox_genre->completionObject();
    comp->insertItems(genres);
    ui.kcombobox_genre->addItems(genres);
    if (genres.isEmpty())
        ui.kcombobox_genre->setToolTip(i18n("No genre list found: genres.json is not installed, or not where Audex looks for its data. Type the genre."));
    connect(ui.kcombobox_genre, &KComboBox::returnPressed, comp, [comp](const QString &text) {
        comp->addItem(text);
    });

    ui.checkBox_various->setChecked(cdda_model->variousArtists());
    connect(ui.checkBox_various, &QAbstractButton::toggled, this, &CDDAHeaderDataDialog::trigger_changed);
    ui.checkBox_multicd->setChecked(cdda_model->cdInfo().metadata().flag(Audex::Metadata::Field::MultiDisc));
    // read-only, shows the result of the HDCD detection (see eventFilter())
    ui.checkBox_hdcd->setChecked(hdcd.value_or(false));
    ui.checkBox_hdcd->setEnabled(hdcd.has_value());
    ui.checkBox_hdcd->setFocusPolicy(Qt::NoFocus);
    ui.checkBox_hdcd->installEventFilter(this);
    if (!hdcd)
        ui.checkBox_hdcd->setToolTip(i18n("Not checked: the HDCD detection is turned off or has not finished yet."));
    else if (*hdcd)
        ui.checkBox_hdcd->setToolTip(i18n("HDCD encoded disc. The bit-perfect rip preserves all HDCD information; see the README for decoding with FFmpeg."));
    else
        ui.checkBox_hdcd->setToolTip(i18n("No HDCD encoding found on this disc."));
    // read-only, the pre-emphasis flags of the TOC: partially checked if only
    // some tracks are flagged
    const Audex::CDInfo &info = cdda_model->cdInfo();
    const qsizetype emphasized = info.preEmphasisTracks().size();
    ui.checkBox_preemphasis->setTristate(true);
    ui.checkBox_preemphasis->setCheckState(emphasized == 0                                     ? Qt::Unchecked
                                               : emphasized == info.audioTrackNumbers().size() ? Qt::Checked
                                                                                               : Qt::PartiallyChecked);
    ui.checkBox_preemphasis->setFocusPolicy(Qt::NoFocus);
    ui.checkBox_preemphasis->installEventFilter(this);
    ui.checkBox_preemphasis->setToolTip(emphasized > 0 ? CDDAHeaderWidget::preEmphasisText(info) : i18n("No track is flagged with pre-emphasis."));
    connect(ui.checkBox_multicd, &QAbstractButton::toggled, this, &CDDAHeaderDataDialog::enable_checkbox_multicd);
    connect(ui.checkBox_multicd, &QAbstractButton::toggled, this, &CDDAHeaderDataDialog::trigger_changed);
    ui.qlineedit_artist->setText(cdda_model->artist());
    connect(ui.qlineedit_artist, &QLineEdit::textEdited, this, &CDDAHeaderDataDialog::trigger_changed);
    ui.qlineedit_title->setText(cdda_model->album());
    connect(ui.qlineedit_title, &QLineEdit::textEdited, this, &CDDAHeaderDataDialog::trigger_changed);
    ui.kintspinbox_cdnum->setValue(cdda_model->discNumber());
    connect(ui.kintspinbox_cdnum, &QSpinBox::valueChanged, this, &CDDAHeaderDataDialog::trigger_changed);
    ui.kintspinbox_trackoffset->setValue(cdda_model->trackNumberOffset());
    connect(ui.kintspinbox_trackoffset, &QSpinBox::valueChanged, this, &CDDAHeaderDataDialog::trigger_changed);
    ui.kcombobox_genre->lineEdit()->setText(cdda_model->genre());
    connect(ui.kcombobox_genre->lineEdit(), &QLineEdit::textEdited, this, &CDDAHeaderDataDialog::trigger_changed);
    {
        bool ok;
        int year = cdda_model->year().toInt(&ok);
        if (ok)
            ui.kintspinbox_year->setValue(year);
        else
            ui.kintspinbox_year->setValue(QDate::currentDate().year());
    }
    connect(ui.kintspinbox_year, &QSpinBox::valueChanged, this, &CDDAHeaderDataDialog::trigger_changed);
    ui.ktextedit_extdata->setText(cdda_model->comment());
    connect(ui.ktextedit_extdata, &QTextEdit::textChanged, this, &CDDAHeaderDataDialog::trigger_changed);
    ui.qlineedit_cddbdiscid->setText(u"0x"_s + cdda_model->cdInfo().cddbDiscId());

    enable_checkbox_multicd(cdda_model->cdInfo().metadata().flag(Audex::Metadata::Field::MultiDisc));

    applyButton->setEnabled(false);
}

bool CDDAHeaderDataDialog::eventFilter(QObject *watched, QEvent *event)
{
    // the HDCD and pre-emphasis check boxes ignore clicks and keys, tooltips still work
    if (watched == ui.checkBox_hdcd || watched == ui.checkBox_preemphasis) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
            return true;
        default:
            break;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void CDDAHeaderDataDialog::slotAccepted()
{
    save();
    accept();
}

void CDDAHeaderDataDialog::slotApplied()
{
    save();
}

void CDDAHeaderDataDialog::save()
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderDataDialog::save() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    cdda_model->setVariousArtists(ui.checkBox_various->isChecked());
    cdda_model->setAlbumValue(Audex::Metadata::Field::MultiDisc, ui.checkBox_multicd->isChecked());
    cdda_model->setArtist(ui.qlineedit_artist->text());
    cdda_model->setAlbum(ui.qlineedit_title->text());
    cdda_model->setDiscNumber(ui.kintspinbox_cdnum->value());
    cdda_model->setTrackNumberOffset(ui.kintspinbox_trackoffset->value());
    cdda_model->setGenre(ui.kcombobox_genre->lineEdit()->text());
    cdda_model->setYear(QString("%1").arg(ui.kintspinbox_year->value()));
    cdda_model->setComment(ui.ktextedit_extdata->toPlainText());
    applyButton->setEnabled(false);
}

void CDDAHeaderDataDialog::trigger_changed()
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderDataDialog::trigger_changed() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    if (ui.checkBox_various->isChecked() != cdda_model->variousArtists()) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.checkBox_multicd->isChecked() != cdda_model->cdInfo().metadata().flag(Audex::Metadata::Field::MultiDisc)) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.qlineedit_artist->text() != cdda_model->artist()) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.qlineedit_title->text() != cdda_model->album()) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.checkBox_various->isChecked())
        if (ui.kintspinbox_cdnum->value() != cdda_model->discNumber()) {
            applyButton->setEnabled(true);
            return;
        }
    if (ui.kintspinbox_trackoffset->value() != cdda_model->trackNumberOffset()) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.kcombobox_genre->lineEdit()->text() != cdda_model->genre()) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.kintspinbox_year->value() != cdda_model->year().toInt()) {
        applyButton->setEnabled(true);
        return;
    }
    if (ui.ktextedit_extdata->toPlainText().split('\n') != cdda_model->comment().split(u'\n')) {
        applyButton->setEnabled(true);
        return;
    }

    applyButton->setEnabled(false);
}

void CDDAHeaderDataDialog::enable_checkbox_multicd(bool enabled)
{
    ui.kintspinbox_cdnum->setEnabled(enabled);
    ui.label_cdnum->setEnabled(enabled);
}
