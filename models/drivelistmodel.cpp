/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivelistmodel.h"

namespace Audex
{

DriveListModel::DriveListModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    Q_UNUSED(parent);
    revert();
}

int DriveListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : drive_list.count();
}

int DriveListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return COLUMN_COUNT;
}

QVariant DriveListModel::data(const QModelIndex &index, int role) const
{
    if ((index.row() < 0) || (index.row() >= drive_list.count()))
        return QVariant();

    if ((index.column() < 0) || (index.column() >= COLUMN_COUNT))
        return QVariant();

    Device::Drive *drive = drive_list.at(index.row());
    if (!drive)
        return QVariant();

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case COLUMN_UDI:
            return drive->getUDI();
        case COLUMN_DISPLAYNAME: {
            QString name = drive->driveInfo().getDisplayName() + QStringLiteral(u" (") + QString::fromLatin1(drive->getBlockDevice()) + QStringLiteral(u")");
            if (drive->discAvailable())
                return QVariant(name);
            else
                return QVariant(i18n("[NODISC] ") + name);
        }
        case COLUMN_COUNT:
            break;
        }
        return QVariant();
    }

    if (role == Qt::DecorationRole)
        return QIcon::fromTheme(DEFAULT_DEVICE_ICON);

    return QVariant();
}

void DriveListModel::update(const DriveList& driveList)
{
    beginResetModel();
    drive_list = driveList;
    endResetModel();
}

}
