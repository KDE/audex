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

#include "config.h"
#include "models/cddamodel.h"
#include "utils/discidcalculator.h"

class CueSheetWriter : public QObject
{
    Q_OBJECT
public:
    explicit CueSheetWriter(CDDAModel *model, const int sampleOffset = 0);
    ~CueSheetWriter() override;

    QStringList cueSheet(const QString &binFilename, const int frameOffset = 0) const;
    QStringList cueSheet(const QStringList &filenames) const;

private:
    CDDAModel *model;

    int sample_offset;

    QString p_filetype(const QString &filename) const;
};

#endif
