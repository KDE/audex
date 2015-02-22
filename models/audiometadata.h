/*
 * AUDEX CDDA EXTRACTOR
 *
 * Copyright (C) 2007-2015 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef AUDIOMETADATA_H
#define AUDIOMETADATA_H

#include <KLocale>

#include "metadata.h"

class AudioMetadata : public Metadata
{

public:
  AudioMetadata();
  using Metadata::operator=;
  using Metadata::operator==;
  using Metadata::operator!=;

  /*see http://xiph.org/vorbis/doc/v-comment.html*/
  enum
  {
    Title = 0, /** Track/Work/Album name **/
    Version, /** The version field may be used to differentiate multiple versions of the same track title in a single collection. (e.g. remix info) **/
    Album, /** The collection name to which this cd/track belongs. Mainly the album title. **/
    Artist, /** The artist generally considered responsible for the work. In popular music this is usually the performing band or singer. For classical
                music it would be the composer. For an audio book it would be the author of the original text. **/
    Number, /** The number of this piece. If part of a specific larger collection or album the discnumber. **/
    Performer, /** The artist(s) who performed the work. In classical music this would be the conductor, orchestra, soloists. In an audio book it would
                   be the actor who did the reading. In popular music this is typically the same as the ARTIST and is omitted. **/
    Copyright, /** Copyright attribution, e.g., '2001 Nobody's Band' or '1999 Jack Moffitt' **/
    License, /** License information, eg, 'All Rights Reserved', 'Any Use Permitted', a URL to a license such as a Creative Commons license
                 ("www.creativecommons.org/blahblah/license.html") or the EFF Open Audio License ('distributed under the terms of the Open
                 Audio License. see http://www.eff.org/IP/Open_licenses/eff_oal.html for details'), etc. **/
    Organization, /** Name of the organization producing the cd (i.e. the 'record label') **/
    Description, /** A short text description of the contents **/
    Comment, /** Comment **/
    Genre, /** A short text indication of music genre **/
    Date, /** Date track/album was recorded **/
    Year, /** Year the track/album was published **/
    Location, /** Location where track/album was recorded **/
    Contact, /** Contact information for the creators or distributors. This could be a URL, an email address, the physical
                 address of the producing label. **/
    ISRC, /** ISRC number for a track; see the ISRC intro page (http://www.ifpi.org/isrc/) for more information on ISRC numbers. **/
    Length, /** Length in seconds. **/
    Filename, /** Filename **/
    Cover, /** Cover image **/
    IsSelected, /** Maybe this is (not) selected for some usecase. **/
    NUM
  };

};

#endif
