/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef COVERBROWSERDIALOG_H
#define COVERBROWSERDIALOG_H

#include <QByteArray>
#include <QIcon>
#include <QPixmap>

#include <QDialog>
#include <QPushButton>

#include "preferences.h"

#include "dialogs/errordialog.h"
#include "utils/coverfetcher.h"

#include "ui_coverbrowserwidgetUI.h"

class CoverBrowserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoverBrowserDialog(QWidget *parent = nullptr);
    ~CoverBrowserDialog() override;

    inline int count()
    {
        return cover_fetcher.count();
    }

public Q_SLOTS:
    void fetchThumbnails(const QString &searchstring, const int fetchCount = 0);
    void startFetchCover(const int no);

Q_SIGNALS:
    void coverFetched(const QByteArray &cover);
    void allCoverThumbnailsFetched();
    void nothingFetched();

private Q_SLOTS:
    void select_this(QListWidgetItem *item);

    void enable_select_button();

    void add_item(const QByteArray &cover, const QString &caption, int no);
    void all_fetched();
    void nothing_fetched();

    void cover_fetched(const QByteArray &cover);

    void error(const QString &description, const QString &solution);

    void slotAccepted();

private:
    Ui::CoverBrowserWidgetUI ui;
    CoverFetcher cover_fetcher;
    QPushButton *okButton;

    void setup();
};

#endif
