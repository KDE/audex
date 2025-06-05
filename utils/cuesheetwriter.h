/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QFileInfo>
#include <QString>
#include <QStringList>

#include <KLocalizedString>

#include "datatypes/cdda.h"

namespace Audex
{

class CUESheetWriter : public QObject
{
    Q_OBJECT
public:
    explicit CUESheetWriter(const CDDA &cdda);
    ~CUESheetWriter() override;

    QStringList cueSheet(const QString &binFilename, const int frameOffset = 0, const bool writeMCN = false, const bool writeISRC = false) const;
    QStringList cueSheet(const QStringList &filenames, const int frameOffset = 0, const bool writeMCN = false, const bool writeISRC = false) const;

private:
    CDDA cdda;

    QString filetype(const QString &filename) const;
};

}
