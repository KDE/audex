/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "driveinfo.h"

namespace Audex
{

const QByteArray DriveInfo::getVendor() const
{
    return get(Vendor, QByteArray()).toByteArray();
}

const QByteArray DriveInfo::getModel() const
{
    return get(Model, QByteArray()).toByteArray();
}

const QByteArray DriveInfo::getRevision() const
{
    return get(Revision, QByteArray()).toByteArray();
}

int DriveInfo::getSampleOffset() const
{
    return get(SampleOffset, 0).toInt();
}

const QString DriveInfo::udi() const
{
    return get(UDI, QString()).toString();
}

const QByteArray DriveInfo::getBlockDevice() const
{
    return get(BlockDevice, QByteArray()).toByteArray();
}

const QString DriveInfo::getDisplayName() const
{
    QRegularExpression rx(QStringLiteral("\\s+"));
    QString vendor = QString::fromLatin1(getVendor().toUpper());
    QString model = QString::fromLatin1(getModel());
    vendor.replace(rx, QChar(u' '));
    model.replace(rx, QChar(u' '));
    return QStringLiteral("%1: %2").arg(vendor).arg(model);
}

void DriveInfo::setCapabilities(const CapabilitySet &capset)
{
    QList<QVariant> caplist;
    for (auto i = capset.cbegin(), end = capset.cend(); i != end; ++i)
        caplist << static_cast<int>(*i);
    set(Capabilities, caplist);
}

const DriveInfo::CapabilitySet DriveInfo::getCapabilities() const
{
    CapabilitySet capset;
    QList<QVariant> caplist = get(Capabilities).toList();
    for (int i = 0; i < caplist.count(); ++i)
        capset.insert(static_cast<Capability>(caplist.at(i).toInt()));
    return capset;
}

QDebug operator<<(QDebug debug, const DriveInfo &drive_info)
{
    QDebugStateSaver saver(debug);

    debug.nospace() << "Vendor: " << drive_info.getVendor() << ", Model: " << drive_info.getModel() << ", Revision: " << drive_info.getRevision() << "\n";
    debug.nospace() << "Capabilities: " << drive_info.getCapabilities() << "\n";

    return debug;
}

}
