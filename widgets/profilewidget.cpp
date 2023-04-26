/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "profilewidget.h"

#include <QDebug>
#include <QFileDialog>
#include <QIcon>

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
    connect(listView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(_update()));
    connect(listView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(mod_profile(const QModelIndex &)));
    connect(kpushbutton_add, SIGNAL(clicked()), this, SLOT(add_profile()));
    connect(kpushbutton_rem, SIGNAL(clicked()), this, SLOT(rem_profile()));
    connect(kpushbutton_mod, SIGNAL(clicked()), this, SLOT(mod_profile()));
    connect(kpushbutton_copy, SIGNAL(clicked()), this, SLOT(copy_profile()));
    connect(kpushbutton_load, SIGNAL(clicked()), this, SLOT(load_profiles()));
    connect(kpushbutton_save, SIGNAL(clicked()), this, SLOT(save_profiles()));
    connect(kpushbutton_init, SIGNAL(clicked()), this, SLOT(init_profiles()));

    kpushbutton_add->setIcon(QIcon::fromTheme("list-add"));
    kpushbutton_rem->setIcon(QIcon::fromTheme("list-remove"));

    kpushbutton_load->setIcon(QIcon::fromTheme("document-open"));
    kpushbutton_save->setIcon(QIcon::fromTheme("document-save"));

    kpushbutton_init->setIcon(QIcon::fromTheme("view-refresh"));

    _update();
}

profileWidget::~profileWidget()
{
}

void profileWidget::_update()
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

    _update();
}

void profileWidget::rem_profile()
{
    if (KMessageBox::warningTwoActions(
            this,
            i18n("Do you really want to delete profile \"%1\"?", profile_model->data(profile_model->index(listView->currentIndex().row(), PROFILE_MODEL_COLUMN_NAME_INDEX)).toString()),
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

    _update();
}

void profileWidget::mod_profile(const QModelIndex &index)
{
    auto *dialog = new ProfileDataDialog(profile_model, index.row(), this);

    dialog->exec();

    delete dialog;

    _update();
}

void profileWidget::mod_profile()
{
    mod_profile(listView->currentIndex());
}

void profileWidget::copy_profile()
{
    profile_model->copy(listView->currentIndex().row());
    profile_model->commit();
    profile_model->sortItems();
    _update();
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
    if (KMessageBox::PrimaryAction ==
        KMessageBox::questionTwoActions(this,
                                        i18n("<p>Do you wish to rescan your system for codecs (Lame, Ogg Vorbis, Flac, etc.)?</p>"
                                             "<p><font style=\"font-style:italic;\">This will attempt to create some sample profiles based upon any found codecs.</font></p>"),
                                        i18n("Codec Scan"),
                                        KStandardGuiItem::ok(),
                                        KStandardGuiItem::cancel())) {
        int sizeBefore = profile_model->rowCount();
        profile_model->autoCreate();
        int diff = profile_model->rowCount() - sizeBefore;
        KMessageBox::information(this, 0 == diff ? i18n("No new codecs found") : i18np("1 new profile added", "%1 new profiles added", diff), i18n("Codec Scan"));
    }
}
