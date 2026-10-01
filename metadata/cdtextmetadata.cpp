/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdtextmetadata.h"

using namespace Qt::StringLiterals;

namespace Audex
{

using Metadata::Field;
namespace Pack = Cdda::CdTextPack;

Metadata::Album albumFromCdTextBlock(const Cdda::CdTextBlock &block, const CDInfo &disc)
{
    Metadata::Album album;

    auto apply = [&block](Metadata::Fields &target, int track, bool isAlbum) {
        target.setText(isAlbum ? Field::Album : Field::Title, block.text(Pack::Title, track).trimmed());
        target.setText(Field::Artist, block.text(Pack::Performer, track).trimmed());
        target.setText(Field::Songwriter, block.text(Pack::Songwriter, track).trimmed());
        target.setText(Field::Composer, block.text(Pack::Composer, track).trimmed());
        target.setText(Field::Arranger, block.text(Pack::Arranger, track).trimmed());
        target.setText(Field::Comment, block.text(Pack::Message, track).trimmed());
        const QString code = block.text(Pack::UpcIsrc, track).trimmed();
        if (isAlbum)
            target.setText(Field::MCN, code);
        else
            target.setText(Field::ISRC, code);
    };

    apply(album, 0, true);
    const QString discId = block.text(Pack::DiscId, 0).trimmed();
    if (!discId.isEmpty())
        album.setCustom(u"CDTEXT_DISCID"_s, discId);

    QString genre = Cdda::cdTextGenreName(block.genreCode);
    if (!block.genreText.isEmpty())
        genre = genre.isEmpty() ? block.genreText : u"%1 (%2)"_s.arg(genre, block.genreText);
    album.setText(Field::Genre, genre);

    // CD-Text track numbers are TOC track numbers
    for (const Cdda::Track &t : disc.toc().tracks) {
        Metadata::Track track;
        apply(track, t.number, false);
        if (!track.isEmpty())
            album.setTrack(t.number, track);
    }

    album.confirm();
    return album;
}

MetadataCandidates candidatesFromCdText(const Cdda::CdText &cdText, const CDInfo &disc, quint8 preferredLanguage)
{
    MetadataCandidates result;
    const Cdda::CdTextBlock *preferred = cdText.preferredBlock(preferredLanguage);
    for (const Cdda::CdTextBlock &block : cdText.blocks) {
        MetadataCandidate c;
        c.provider = u"cdtext"_s;
        c.providerName = u"CD-Text"_s;
        c.reference = u"block %1, %2"_s.arg(block.block).arg(Cdda::cdTextLanguageName(block.languageCode));
        c.score = (&block == preferred) ? 60 : 50;
        c.album = albumFromCdTextBlock(block, disc);
        if (!c.album.isEmpty())
            result.append(c);
    }
    sortCandidates(result);
    return result;
}

}
