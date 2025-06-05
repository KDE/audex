/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QDebug>
#include <QList>
#include <QRegularExpression>
#include <QSet>
#include <QString>

#include <KLocalizedString>

#include "dataset.h"

namespace Audex
{

class DriveInfo : public Dataset
{
public:
    enum Type {
        Vendor = 0,
        Model,
        Revision,
        Capabilities,
        SampleOffset,
        UDI,
        BlockDevice,
        Invalid
    };

    enum Capability {
        C2_ERR_PTRS = 0,
        READ_MCN,
        READ_ISRC,
        READ_CDDA,
        READ_CDDA_ACCURATE
    };
    typedef QSet<Capability> CapabilitySet;

    const QByteArray getVendor() const;
    const QByteArray getModel() const;
    const QByteArray getRevision() const;
    int getSampleOffset() const;

    const QString udi() const;
    const QByteArray getBlockDevice() const;

    const QString getDisplayName() const;

    void setCapabilities(const CapabilitySet &capset);
    const CapabilitySet getCapabilities() const;
};

QDebug operator<<(QDebug debug, const DriveInfo &drive_info);

}
