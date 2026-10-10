/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tagwriter.h"

#include <QBuffer>
#include <QColorSpace>
#include <QCoreApplication>
#include <QFile>
#include <QImage>
#include <QImageWriter>
#include <QPainter>

#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4coverart.h>
#include <taglib/mp4file.h>
#include <taglib/mpegfile.h>
#include <taglib/opusfile.h>
#include <taglib/taglib.h>
#include <taglib/tpropertymap.h>
#include <taglib/vorbisfile.h>
#include <taglib/xiphcomment.h>

using namespace Qt::StringLiterals;

namespace Audex::Encoding
{

using Metadata::Field;

namespace
{

QString tr(const char *text)
{
    return QCoreApplication::translate("Audex::TagWriter", text);
}

bool fail(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

}

// ---- mapping (independent of TagLib) --------------------------------------------------

QMap<QString, QStringList> TagWriter::properties(const TagInfo &info)
{
    QMap<QString, QStringList> p;
    const Metadata::Album &a = info.album;
    const bool image = info.trackNumber < 0;
    const Metadata::Track &t = info.track();

    auto put = [&p](const QString &key, const QString &value) {
        if (!value.trimmed().isEmpty())
            p.insert(key, {value});
    };
    auto trackOrAlbum = [&](Field field) {
        const QString v = image ? QString() : t.text(field);
        return v.isEmpty() ? a.text(field) : v;
    };

    const bool various = a.flag(Field::VariousArtists);
    QString albumArtist = a.text(Field::Artist);
    if (albumArtist.isEmpty() && various)
        albumArtist = u"Various Artists"_s;

    if (image) {
        put(u"TITLE"_s, a.text(Field::Album));
        put(u"ARTIST"_s, albumArtist);
    } else {
        put(u"TITLE"_s, t.text(Field::Title));
        put(u"ARTIST"_s, trackOrAlbum(Field::Artist));
        if (info.displayTrackNumber > 0 || info.trackNumber > 0)
            put(u"TRACKNUMBER"_s, QString::number(info.displayTrackNumber));
        if (info.trackTotal > 0)
            put(u"TRACKTOTAL"_s, QString::number(info.trackTotal));
        put(u"ISRC"_s, t.text(Field::ISRC));
        put(u"MUSICBRAINZ_TRACKID"_s, t.text(Field::MusicBrainzRecordingId));
        put(u"MUSICBRAINZ_RELEASETRACKID"_s, t.text(Field::MusicBrainzTrackId));
        put(u"MUSICBRAINZ_ARTISTID"_s, t.text(Field::MusicBrainzArtistId).isEmpty() ? a.text(Field::MusicBrainzArtistId) : t.text(Field::MusicBrainzArtistId));
    }

    put(u"ALBUM"_s, a.text(Field::Album));
    put(u"ALBUMARTIST"_s, albumArtist);
    put(u"DATE"_s, a.text(Field::Year));
    put(u"GENRE"_s, trackOrAlbum(Field::Genre));
    put(u"COMMENT"_s, trackOrAlbum(Field::Comment));
    put(u"COMPOSER"_s, trackOrAlbum(Field::Composer));
    put(u"SONGWRITER"_s, trackOrAlbum(Field::Songwriter));
    put(u"ARRANGER"_s, trackOrAlbum(Field::Arranger));
    put(u"PERFORMER"_s, trackOrAlbum(Field::Performer));
    put(u"WORK"_s, trackOrAlbum(Field::Work));
    put(u"LABEL"_s, a.text(Field::Label));
    put(u"CATALOGNUMBER"_s, a.text(Field::CatalogNumber));
    put(u"BARCODE"_s, a.text(Field::Barcode));
    put(u"RELEASECOUNTRY"_s, a.text(Field::Country));
    put(u"DISCSUBTITLE"_s, a.text(Field::DiscSubtitle));
    put(u"MUSICBRAINZ_ALBUMID"_s, a.text(Field::MusicBrainzReleaseId));
    put(u"MUSICBRAINZ_RELEASEGROUPID"_s, a.text(Field::MusicBrainzReleaseGroupId));
    put(u"MUSICBRAINZ_ALBUMARTISTID"_s, a.text(Field::MusicBrainzArtistId));
    put(u"MUSICBRAINZ_DISCID"_s, a.text(Field::MusicBrainzDiscId));
    if (a.number(Field::DiscNumber) > 0) {
        put(u"DISCNUMBER"_s, QString::number(a.number(Field::DiscNumber)));
        if (a.number(Field::DiscCount) > 0)
            put(u"DISCTOTAL"_s, QString::number(a.number(Field::DiscCount)));
    }
    if (various)
        put(u"COMPILATION"_s, u"1"_s);
    for (auto it = info.extra.cbegin(); it != info.extra.cend(); ++it)
        if (!it.key().trimmed().isEmpty())
            put(it.key().trimmed().toUpper(), it.value());
    return p;
}

namespace
{

bool isOpaque(const QImage &image)
{
    const QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < argb.height(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(argb.constScanLine(y));
        for (int x = 0; x < argb.width(); ++x)
            if (qAlpha(line[x]) != 255)
                return false;
    }
    return true;
}

}

std::optional<PreparedCover> TagWriter::prepareCover(const Metadata::CoverArt &cover, int maxSize, CoverFormat format)
{
    if (cover.isNull())
        return std::nullopt;
    QImage image = QImage::fromData(cover.data);
    if (image.isNull())
        return std::nullopt;

    PreparedCover result;
    const bool isJpeg = cover.mimeType == u"image/jpeg";
    const bool isPng = cover.mimeType == u"image/png";
    const bool png = format == CoverFormat::Png || (format == CoverFormat::Original && (isPng || (!isJpeg && image.hasAlphaChannel())));
    const bool small = maxSize <= 0 || (image.width() <= maxSize && image.height() <= maxSize);
    if (small && (png ? isPng : isJpeg)) {
        result.cover = cover;
    } else {
        if (!small)
            image = image.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        // players expect sRGB; a profile only costs space
        if (image.colorSpace().isValid() && image.colorSpace() != QColorSpace(QColorSpace::SRgb))
            image.convertToColorSpace(QColorSpace(QColorSpace::SRgb));
        image.setColorSpace(QColorSpace());
        if (image.hasAlphaChannel() && (!png || isOpaque(image))) { // JPEG has no transparency
            QImage opaque(image.size(), QImage::Format_RGB32);
            opaque.fill(Qt::white);
            QPainter(&opaque).drawImage(0, 0, image);
            image = opaque;
        }
        if (image.format() != QImage::Format_Grayscale8 && image.allGray())
            image.convertTo(QImage::Format_Grayscale8);
        QByteArray data;
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        QImageWriter writer(&buffer, png ? "png" : "jpeg");
        if (!png) {
            // 80 is a smaller than 90 at hardly visible cost; baseline
            // (not progressive), as some car radios and players need it
            writer.setQuality(80);
            writer.setOptimizedWrite(true);
        }
        if (!writer.write(image))
            return std::nullopt;
        result.cover.data = data;
        result.cover.mimeType = png ? u"image/png"_s : u"image/jpeg"_s;
        result.cover.source = cover.source;
    }
    result.width = image.width();
    result.height = image.height();
    result.depth = image.depth();
    return result;
}

// ---- TagLib ----------------------------------------------------------------------------

namespace
{

TagLib::String toTag(const QString &s)
{
    return TagLib::String(s.toUtf8().constData(), TagLib::String::UTF8);
}

TagLib::ByteVector toBytes(const QByteArray &b)
{
    return TagLib::ByteVector(b.constData(), uint(b.size()));
}

TagLib::PropertyMap toPropertyMap(const QMap<QString, QStringList> &properties)
{
    TagLib::PropertyMap map;
    for (auto it = properties.cbegin(); it != properties.cend(); ++it) {
        TagLib::StringList values;
        for (const QString &v : it.value())
            values.append(toTag(v));
        map.insert(toTag(it.key()), values);
    }
    return map;
}

// ID3v2 and MP4 keep the total in the number field ("3/12")
QMap<QString, QStringList> combinedNumbers(QMap<QString, QStringList> p)
{
    auto combine = [&p](const QString &numberKey, const QString &totalKey) {
        if (p.contains(numberKey) && p.contains(totalKey))
            p[numberKey] = QStringList{p.value(numberKey).first() + u'/' + p.value(totalKey).first()};
        p.remove(totalKey);
    };
    combine(u"TRACKNUMBER"_s, u"TRACKTOTAL"_s);
    combine(u"DISCNUMBER"_s, u"DISCTOTAL"_s);
    return p;
}

TagLib::FLAC::Picture *flacPicture(const PreparedCover &c)
{
    auto *picture = new TagLib::FLAC::Picture;
    picture->setType(TagLib::FLAC::Picture::FrontCover);
    picture->setMimeType(toTag(c.cover.mimeType));
    picture->setData(toBytes(c.cover.data));
    picture->setWidth(c.width);
    picture->setHeight(c.height);
    picture->setColorDepth(c.depth);
    return picture;
}

bool saveXiph(TagLib::File &file, TagLib::Ogg::XiphComment *comment, const TagInfo &info, const std::optional<PreparedCover> &cover, QString *error)
{
    if (!comment)
        return fail(error, tr("The file has no Vorbis comment block."));
    file.setProperties(toPropertyMap(TagWriter::properties(info)));
    comment->removeAllPictures();
    if (cover)
        comment->addPicture(flacPicture(*cover));
    return file.save() || fail(error, tr("Cannot save the tags."));
}

}

bool TagWriter::supportsSuffix(const QString &suffix)
{
    static const QStringList supported{u"flac"_s, u"mp3"_s, u"opus"_s, u"ogg"_s, u"oga"_s, u"m4a"_s, u"mp4"_s, u"m4b"_s};
    return supported.contains(suffix.toLower());
}

bool TagWriter::write(const QString &path, const QString &suffix, const TagInfo &info, QString *error)
{
    const QString s = suffix.toLower();
    if (!supportsSuffix(s))
        return fail(error, tr("Tags are not supported for .%1 files.").arg(suffix));

    const QByteArray name = QFile::encodeName(path);
    const std::optional<PreparedCover> cover = info.embedCover ? prepareCover(info.album.cover(), info.coverMaxSize, info.coverFormat) : std::nullopt;

    if (s == u"flac") {
        TagLib::FLAC::File file(name.constData());
        if (!file.isValid())
            return fail(error, tr("Cannot read %1 for tagging.").arg(path));
        file.xiphComment(true);
        file.setProperties(toPropertyMap(properties(info)));
        file.removePictures();
        if (cover)
            file.addPicture(flacPicture(*cover));
        return file.save() || fail(error, tr("Cannot save the tags."));
    }

    if (s == u"opus") {
        TagLib::Ogg::Opus::File file(name.constData());
        if (!file.isValid())
            return fail(error, tr("Cannot read %1 for tagging.").arg(path));
        return saveXiph(file, file.tag(), info, cover, error);
    }

    if (s == u"ogg" || s == u"oga") {
        TagLib::Ogg::Vorbis::File file(name.constData());
        if (!file.isValid())
            return fail(error, tr("Cannot read %1 for tagging.").arg(path));
        return saveXiph(file, file.tag(), info, cover, error);
    }

    if (s == u"mp3") {
        TagLib::MPEG::File file(name.constData());
        if (!file.isValid())
            return fail(error, tr("Cannot read %1 for tagging.").arg(path));
        TagLib::ID3v2::Tag *tag = file.ID3v2Tag(true);
        tag->setProperties(toPropertyMap(combinedNumbers(properties(info))));
        tag->removeFrames("APIC");
        if (cover) {
            auto *frame = new TagLib::ID3v2::AttachedPictureFrame;
            frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
            frame->setMimeType(toTag(cover->cover.mimeType));
            frame->setPicture(toBytes(cover->cover.data));
            tag->addFrame(frame);
        }
        return file.save(TagLib::MPEG::File::ID3v2, TagLib::File::StripOthers, TagLib::ID3v2::v4) || fail(error, tr("Cannot save the tags."));
    }

    // MP4 container
    TagLib::MP4::File file(name.constData());
    if (!file.isValid())
        return fail(error, tr("Cannot read %1 for tagging.").arg(path));
    TagLib::MP4::Tag *tag = file.tag();
    const TagLib::PropertyMap unsupported = tag->setProperties(toPropertyMap(combinedNumbers(properties(info))));
    // keys without an MP4 atom of their own (e.g. the pre-emphasis tag)
    // become iTunes freeform items, as other taggers write them
    for (auto it = unsupported.begin(); it != unsupported.end(); ++it)
        tag->setItem(TagLib::String("----:com.apple.iTunes:") + it->first, TagLib::MP4::Item(it->second));
    if (cover) {
        TagLib::MP4::CoverArtList list;
        list.append(TagLib::MP4::CoverArt(cover->cover.mimeType == u"image/png" ? TagLib::MP4::CoverArt::PNG : TagLib::MP4::CoverArt::JPEG,
                                          toBytes(cover->cover.data)));
        tag->setItem("covr", TagLib::MP4::Item(list));
    } else {
        tag->removeItem("covr");
    }
    return file.save() || fail(error, tr("Cannot save the tags."));
}

}
