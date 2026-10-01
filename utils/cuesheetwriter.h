/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QFileInfo>
#include <QString>
#include <QStringList>

#include "core/subchannel.h"
#include "metadata/cdinfo.h"

// Writes cue sheets for the ripped files. Positions inside a single image
// file are relative to the start of the first written track.
class CueSheetWriter
{
public:
    explicit CueSheetWriter(const Audex::CDInfo &info);
    ~CueSheetWriter();

    // pregaps and indexes; ISRC and MCN where the metadata has none
    void setSubchannel(const Audex::Cdda::SubchannelScan &scan);

    // single image file (single file rip); tracks = written TOC track numbers
    QStringList cueSheet(const QString &binFilename, const QList<int> &tracks, const bool writeMCN = false, const bool writeISRC = false) const;

    // one audio file per track
    QStringList cueSheet(const QStringList &filenames, const QList<int> &tracks, const bool writeMCN = false, const bool writeISRC = false) const;

private:
    QStringList header(const bool writeMCN) const;
    QStringList trackLines(const int number, const bool writeISRC) const;

    const QString p_filetype(const QString &filename) const;

    Audex::CDInfo info;
    Audex::Cdda::SubchannelScan subchannel;
};
