/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DRIVELISTMODEL_H
#define DRIVELISTMODEL_H

#include <QAbstractTableModel>
#include <QIcon>

#include "device/drive.h"

#define DEFAULT_DEVICE_ICON QStringLiteral("drive-optical")

namespace Audex
{

class DriveListModel : public QAbstractTableModel
{
public:
    explicit DriveListModel(QObject *parent = nullptr);

    enum Columms { COLUMN_UDI = 0, COLUMN_DISPLAYNAME, COLUMN_COUNT };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public Q_SLOTS:
    void update(const DriveList& driveList);

private:
    DriveList drive_list;
};

}

#endif
