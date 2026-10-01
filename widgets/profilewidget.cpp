/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "profilewidget.h"

#include "dialogs/profiledatadialog.h"
#include "models/profilemodel.h"

#include <KMessageBox>

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QIcon>
#include <QPushButton>
#include <QSize>

profileWidget::profileWidget(ProfileModel *profileModel, QWidget *parent)
    : profileWidgetUI(parent)
    , profile_model(profileModel)
{
    if (!profile_model) {
        qWarning() << "profileWidget() called with null model pointers";
        Q_ASSERT(profile_model);
        return;
    }

    listView->setModel(profile_model);
    listView->setModelColumn(1);
    listView->setIconSize(QSize(22, 22));
    connect(listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &profileWidget::p_update);
    connect(listView, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex &index) {
        mod_profile(index);
    });
    connect(kpushbutton_add, &QAbstractButton::clicked, this, &profileWidget::add_profile);
    connect(kpushbutton_rem, &QAbstractButton::clicked, this, &profileWidget::rem_profile);
    connect(kpushbutton_mod, &QPushButton::clicked, this, [this]() {
        mod_profile();
    });
    connect(kpushbutton_copy, &QAbstractButton::clicked, this, &profileWidget::copy_profile);
    connect(kpushbutton_load, &QAbstractButton::clicked, this, &profileWidget::load_profiles);
    connect(kpushbutton_save, &QAbstractButton::clicked, this, &profileWidget::save_profiles);

    kpushbutton_add->setIcon(QIcon::fromTheme("list-add"));
    kpushbutton_rem->setIcon(QIcon::fromTheme("list-remove"));

    kpushbutton_load->setIcon(QIcon::fromTheme("document-open"));
    kpushbutton_save->setIcon(QIcon::fromTheme("document-save"));

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
    ProfileDataDialog dialog(profile_model, -1, this);

    if (dialog.exec() == QDialog::Accepted) {
        profile_model->sortItems();
        p_update();
    }
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

void profileWidget::mod_profile(const QModelIndex &index)
{
    ProfileDataDialog dialog(profile_model, index.row(), this);
    if (dialog.exec() == QDialog::Accepted)
        p_update();
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
    p_update();
}

void profileWidget::save_profiles()
{
    const QString filename = QFileDialog::getSaveFileName(this, i18n("Save Cover"), QDir::homePath(), "*.apf");
    if (!filename.isEmpty()) {
        profile_model->saveProfilesToFile(filename);
    }
}

void profileWidget::load_profiles()
{
    const QString filename = QFileDialog::getOpenFileName(this, i18n("Load Profiles"), QDir::homePath(), "*.apf");
    if (!filename.isEmpty()) {
        profile_model->loadProfilesFromFile(filename);
    }
}
