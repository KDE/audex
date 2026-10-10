/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "metadata/metadata.h"

#include <QMap>
#include <QStringList>

#include <optional>

namespace Audex::Encoding
{

// Format of an embedded cover; Original keeps JPEG and PNG as they come
enum class CoverFormat {
    Original,
    Jpeg,
    Png
};

// Everything the tagger needs for one output file
struct TagInfo {
    Metadata::Album album;
    int trackNumber = 0; // TOC number, 0 = hidden track; -1 = whole disc (image)
    int displayTrackNumber = 0; // with track number offset
    int trackTotal = 0;
    bool embedCover = true;
    int coverMaxSize = 1000; // pixels, 0 = keep original
    CoverFormat coverFormat = CoverFormat::Original;
    QMap<QString, QString> extra; // further tags, e.g. the pre-emphasis flag

    const Metadata::Track &track() const
    {
        return album.track(trackNumber);
    }
};

struct PreparedCover {
    Metadata::CoverArt cover;
    int width = 0;
    int height = 0;
    int depth = 0;
};

// Writes tags with TagLib after encoding, identical for all formats:
// ID3v2.4 for MP3, Vorbis comments for FLAC/Opus/Vorbis, MP4 atoms for M4A.
class TagWriter
{
public:
    static bool supportsSuffix(const QString &suffix);

    static bool write(const QString &path, const QString &suffix, const TagInfo &info, QString *error = nullptr);

    // Tag names follow the Vorbis comment / Picard conventions; TagLib maps
    // them to the frames of each format.
    static QMap<QString, QStringList> properties(const TagInfo &info);

    // Keeps the original bytes if they have the wanted format and are small
    // enough, otherwise scales (keeping the aspect ratio) and re-encodes.
    // Original writes other formats as JPEG, or PNG if they are transparent.
    static std::optional<PreparedCover> prepareCover(const Metadata::CoverArt &cover, int maxSize, CoverFormat format = CoverFormat::Original);
};

}
