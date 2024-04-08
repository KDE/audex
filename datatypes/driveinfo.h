/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DRIVEINFO_H
#define DRIVEINFO_H

#include <QByteArray>
#include <QDebug>
#include <QRegularExpression>
#include <QSet>
#include <QList>
#include <QString>

#include <KLocalizedString>

#include "dataset.h"

namespace Audex
{

class DriveInfo : public Dataset
{
public:
    enum Type { Vendor = 0, Model, Revision, Capabilities, SampleOffset, Invalid };

    enum Capability { C2_ERR_PTRS = 0, READ_MCN, READ_ISRC, READ_CDDA, READ_CDDA_ACCURATE };
    typedef QSet<Capability> CapabilitySet;

    const QByteArray getVendor() const;
    const QByteArray getModel() const;
    const QByteArray getRevision() const;
    int getSampleOffset() const;

    const QString getDisplayName() const;

    void setCapabilities(const CapabilitySet &capset);
    const CapabilitySet getCapabilities() const;

};

typedef QList<DriveInfo> DriveInfoList;

QDebug operator<<(QDebug debug, const DriveInfo &drive_info);

}

#endif
