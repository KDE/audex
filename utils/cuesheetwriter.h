/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CUESHEETWRITER_H
#define CUESHEETWRITER_H

#include <QFileInfo>
#include <QString>
#include <QStringList>

#include <KLocalizedString>

#include "../models/cddamodel.h"
#include "config.h"

#define FRAMES2SEC(frames) ((frames) / 75)

class CueSheetWriter : public QObject
{
    Q_OBJECT
public:
    explicit CueSheetWriter(CDDAModel *model);
    ~CueSheetWriter() override;

    QStringList cueSheet(const QString &binFilename) const;
    QStringList cueSheet(const QStringList &filenames) const;

private:
    CDDAModel *model;

    QString p_filetype(const QString &filename) const;
};

#endif
