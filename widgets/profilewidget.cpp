/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profilewidget.h"

#include <QDebug>
#include <QFileDialog>
#include <QIcon>

namespace Audex
{

profileWidget::profileWidget(ProfileModel *profileModel, QWidget *parent)
    : profileWidgetUI(parent)
{
    profile_model = profileModel;
    if (!profile_model) {
        qDebug() << "ProfileModel is NULL!";
        return;
    }

    listView->setModel(profile_model);
    listView->setModelColumn(1);
    listView->setIconSize(QSize(22, 22));

    QObject::connect(listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &profileWidget::p_update);
    QObject::connect(listView, &QListView::doubleClicked, this, &profileWidget::mod_profile_index);

    QObject::connect(kpushbutton_add, &QPushButton::clicked, this, &profileWidget::add_profile);
    QObject::connect(kpushbutton_rem, &QPushButton::clicked, this, &profileWidget::rem_profile);
    QObject::connect(kpushbutton_mod, &QPushButton::clicked, this, &profileWidget::mod_profile);
    QObject::connect(kpushbutton_copy, &QPushButton::clicked, this, &profileWidget::copy_profile);
    QObject::connect(kpushbutton_load, &QPushButton::clicked, this, &profileWidget::load_profiles);
    QObject::connect(kpushbutton_save, &QPushButton::clicked, this, &profileWidget::save_profiles);
    QObject::connect(kpushbutton_init, &QPushButton::clicked, this, &profileWidget::init_profiles);

    kpushbutton_add->setIcon(QIcon::fromTheme("list-add"));
    kpushbutton_rem->setIcon(QIcon::fromTheme("list-remove"));

    kpushbutton_load->setIcon(QIcon::fromTheme("document-open"));
    kpushbutton_save->setIcon(QIcon::fromTheme("document-save"));

    kpushbutton_init->setIcon(QIcon::fromTheme("view-refresh"));

    p_update();
}

profileWidget::~profileWidget()
{
}

void profileWidget::p_update()
{
    kpushbutton_rem->setEnabled(listView->selectionModel()->selectedIndexes().count() > 0);
    kpushbutton_mod->setEnabled(listView->selectionModel()->selectedIndexes().count() > 0);
    kpushbutton_copy->setEnabled(listView->selectionModel()->selectedIndexes().count() > 0);
    kpushbutton_save->setEnabled(profile_model->rowCount() > 0);
}

void profileWidget::add_profile()
{
    auto *dialog = new ProfileDataDialog(profile_model, -1, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }
    delete dialog;

    profile_model->sortItems();

    p_update();
}

void profileWidget::rem_profile()
{
    if (KMessageBox::warningTwoActions(
            this,
            i18n("Do you really want to delete profile \"%1\"?",
                 profile_model->data(profile_model->index(listView->currentIndex().row(), PROFILE_MODEL_COLUMN_NAME_INDEX)).toString()),
            i18n("Delete profile"),
            KStandardGuiItem::ok(),
            KStandardGuiItem::cancel())
        == KMessageBox::SecondaryAction)
        return;

    QModelIndex ci = listView->currentIndex();
    profile_model->removeRows(ci.row(), 1);

    profile_model->commit();

    if (ci.isValid())
        listView->setCurrentIndex(ci);

    p_update();
}

void profileWidget::mod_profile_index(const QModelIndex &index)
{
    QPointer<ProfileDataDialog> dialog = new ProfileDataDialog(profile_model, index.row(), this);
    dialog->exec();
    p_update();
}

void profileWidget::mod_profile()
{
    mod_profile_index(listView->currentIndex());
}

void profileWidget::copy_profile()
{
    profile_model->copy(listView->currentIndex().row());
    profile_model->commit();
    profile_model->sortItems();
    p_update();
}

void profileWidget::save_profiles()
{
    QString filename = QFileDialog::getSaveFileName(this, i18n("Save Cover"), QDir::homePath(), "*.apf");
    if (!filename.isEmpty()) {
        profile_model->saveProfilesToFile(filename);
    }
}

void profileWidget::load_profiles()
{
    QString filename = QFileDialog::getOpenFileName(this, i18n("Load Profiles"), QDir::homePath(), "*.apf");
    if (!filename.isEmpty()) {
        profile_model->loadProfilesFromFile(filename);
    }
}

void profileWidget::init_profiles()
{
    if (KMessageBox::PrimaryAction
        == KMessageBox::questionTwoActions(
            this,
            i18n("<p>Do you wish to rescan your system for codecs (Lame, Opus, FLAC, etc.)?</p>"
                 "<p><font style=\"font-style:italic;\">This will attempt to create some sample profiles based upon any found codecs.</font></p>"),
            i18n("Codec Scan"),
            KStandardGuiItem::ok(),
            KStandardGuiItem::cancel())) {
        int sizeBefore = profile_model->rowCount();
        profile_model->autoCreate();
        int diff = profile_model->rowCount() - sizeBefore;
        KMessageBox::information(this,
                                 0 == diff ? i18n("No new codecs found") : i18np("1 new profile added", "%1 new profiles added", diff),
                                 i18n("Codec Scan"));
    }
}

}
