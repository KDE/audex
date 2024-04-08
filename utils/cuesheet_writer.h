/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CUESHEET_WRITER_H
#define CUESHEET_WRITER_H

#include <QFileInfo>
#include <QString>
#include <QStringList>

#include <KLocalizedString>

#include "datatypes/cdinfo.h"

#include "cddbid.h"

namespace Audex
{

class CUESheetWriter : public QObject
{
    Q_OBJECT
public:
    explicit CUESheetWriter(const CDInfo &cdInfo);
    ~CUESheetWriter() override;

    QStringList cueSheet(const QString &binFilename, const int frameOffset = 0, const bool writeMCN = false, const bool writeISRC = false) const;
    QStringList cueSheet(const QStringList &filenames, const int frameOffset = 0, const bool writeMCN = false, const bool writeISRC = false) const;

private:
    CDInfo cdinfo;

    QString filetype(const QString &filename) const;
};

}

#endif
